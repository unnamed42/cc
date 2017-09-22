#include "text/ustring.hpp"
#include "utils/ptrlist.hpp"
#include "lexical/token.hpp"
#include "lexical/tokentype.hpp"
#include "semantic/typeenum.hpp"
#include "semantic/type.hpp"
#include "semantic/expr.hpp"
#include "semantic/decl.hpp"
#include "semantic/opcode.hpp"
#include "semantic/scope.hpp"
#include "semantic/parser.hpp"
#include "diagnostic/logger.hpp"
#include "constexpr/evaluator.hpp"

#include <cassert>

namespace impl = Compiler::Semantic;

using namespace Compiler::Text;
using namespace Compiler::Utils;
using namespace Compiler::Lexical;
using namespace Compiler::Semantic;
using namespace Compiler::ConstExpr;
using namespace Compiler::Diagnostic;

/**
 * RAII trick to declare new scope and auto exit
 */
struct ScopeGuard {
    Scope* &curr;
    Scope  new_;
    Scope *save;
    bool   done;
    
    ScopeGuard(Scope* &curr, ScopeType type = BlockScope) :
        curr(curr), new_(type, curr), save(&new_), done(false) { std::swap(this->curr, save); }
    ~ScopeGuard() noexcept { if(!done) std::swap(curr, save); }
    
    void release() noexcept { std::swap(save, curr); done = true; }
};

static unsigned precedence(TokenType type) noexcept {
    switch(type) {
        case Star: case Div: case Mod: return 10;
        case Add: case Sub: return 9;
        case LeftShift: case RightShift: return 8;
        case LessThan: case GreaterThan: 
        case LessEqual: case GreaterEqual: return 7;
        case Equal: case NotEqual: return 6;
        case Ampersand: return 5;
        case BitXor: return 4;
        case BitOr: return 3;
        case LogicalAnd: return 2;
        case LogicalOr: return 1;
        default: return 0;
    }
}

SourceLoc *impl::epos = nullptr;

Parser::Parser(const char *path) : m_src(path), m_file(new Scope(FileScope)), m_curr(m_file) {}

Parser::~Parser() noexcept { 
    delete m_file; 
}

Token* Parser::get() {
    auto ret = m_src.get();
    if(!ret)
        derr << "unexpected end of file";
    return ret;
}

Token* Parser::peek() {
    auto ret = m_src.peek();
    if(!ret)
        derr << "unexpected end of file";
    return ret;
}

void Parser::markPos(Token *tok) {
    epos = tok->sourceLoc();
}

void Parser::parse() {
    ;
}

bool Parser::isSpecifier(Token *name) {
    if(isTypeSpecifier(name->type()))
        return true;
    auto decl = m_curr->find(name);
    return decl && decl->storageClass() == Typedef;
}

/*------------------------------.
|   primary_expression          |
|       : IDENTIFIER            |
|       | CONSTANT              |
|       | STRING_LITERAL        |
|       | '(' expression ')'    |
|       | 'true'                | // my addition
|       | 'false'               |
|       ;                       |
`------------------------------*/
Expr* Parser::primaryExpr() {
    auto tok = get();
    switch(tok->type()) {
        case Identifier: return makeIdentifier(tok);
        case String: return makeString(tok);
        case Character: return makeChar(tok); //
        case PPNumber: case PPFloat: return makeNumber(tok);
        case LeftParen: {
            auto res = expr(); m_src.expect(RightParen); return res;
        }
        case KeyTrue: case KeyFalse: return makeBool(tok);
        default: 
            derr << tok->sourceLoc()
                << "expecting a primary expression, but get " << tok->toString();
    }
    assert(false);
}

/*---------------------------------------------------------------.
|   postfix_expression                                           |
|       : primary_expression                                     |
|       | postfix_expression '[' expression ']'                  |
|       | postfix_expression '(' ')'                             |
|       | postfix_expression '(' argument_expression_list ')'    |
|       | postfix_expression '.' IDENTIFIER                      |
|       | postfix_expression PTR_OP IDENTIFIER                   |
|       | postfix_expression INC_OP                              |
|       | postfix_expression DEC_OP                              |
$       | '(' type_name ')' '{' initializer_list '}'             $ // TODO
$       | '(' type_name ')' '{' initializer_list ',' '}'         $ // TODO
|       ;                                                        |
`---------------------------------------------------------------*/
Expr* Parser::postfixExpr() {
    auto result = primaryExpr();
    for(;;) {
        auto tok = get();
        markPos(tok);
        switch(tok->type()) {
            case LeftSubscript:
                result = makeBinary(tok, OpSubscript, result, expr());
                m_src.expect(RightSubscript);
                break;
            case LeftParen: {
                auto resTok = result->token();
                auto func = m_curr->find(resTok);
                auto funcDecl = func ? func->toFunc() : nullptr;
                if(!funcDecl)
                    derr << resTok->sourceLoc()
                        << "a function designator required";
                result = makeCall(funcDecl, argumentExprList());
                //m_src.expect(RightParen); // handled in args
                break;
            }
            case Inc: result = makeUnary(tok, OpPostfixInc, result); break;
            case Dec: result = makeUnary(tok, OpPostfixDec, result); break;
            case Dot: case MemberPtr: 
                // symbol resolution is handled in make function
                result = makeMemberAccess(tok, result, m_src.get()); break;
            default: m_src.unget(tok); return result;
        }
    }
}

/*------------------------------------------------.
|   expression                                    |
|       : assignment_expression                   |
|       | expression ',' assignment_expression    |
|       ;                                         |
`------------------------------------------------*/
Expr* Parser::expr() {
    auto result = assignmentExpr();
    Token *tok;
    while((tok = m_src.want(Comma)))
        result = makeBinary(tok, OpComma, result, assignmentExpr());
    return result;
}

// same definition as expression
PtrList Parser::argumentExprList() {
    PtrList list{};
    while(!m_src.test(RightParen)) {
        list.pushBack(assignmentExpr());
        if(!m_src.test(Comma)) {
            m_src.expect(RightParen);
            break;
        }
    }
    return list;
}

/*---------------------------------------------------------------------------.
|   assignment_expression                                                    |
|       : conditional_expression                                             |
$       | unary_expression ASSIGNMENT_OPERATOR assignment_expression         $ // C99 standard
|       | logical_or_expression ASSIGNMENT_OPERATOR assignment_expression    | // simplified version
|       ;                                                                    |
`---------------------------------------------------------------------------*/
Expr* Parser::assignmentExpr() {
    auto result = binaryExpr();
    auto tok = get();
    
    if(tok->is(Question)) {
        auto yes = expr();
        m_src.expect(Colon);
        auto no = conditionalExpr();
        return makeTernary(tok, result, yes, no);
    } else if(isAssignment(tok->type())) 
        return makeAssignment(tok, result, assignmentExpr());
    else {
        m_src.unget(tok);
        return result;
    }
}

/*------------------------------------------. /+--------------------.
|   unary_expression                        | |   unary_operator    |
|       : postfix_expression                | |       : '&'         |
|       | INC_OP unary_expression           | |       | '*'         |
|       | DEC_OP unary_expression           | |       | '+'         |
|       | unary_operator cast_expression    | |       | '-'         |
|       | SIZEOF unary_expression           | |       | '~'         |
|       | SIZEOF '(' type_name ')'          | |       | '!'         |
|       ;                                   | |       ;             |
`------------------------------------------+/ `--------------------*/
Expr* Parser::unaryExpr() {
    OpCode op;
    auto tok = get();
    switch(tok->type()) {
        case Dec: op = OpPrefixDec; break; 
        case Inc: op = OpPrefixInc; break;
        case Ampersand: op = OpAddressOf; break;
        case Star: op = OpObjectOf; break;
        case Add: op = OpValueOf; break;
        case Sub: op = OpNegate; break;
        case KeySizeof: 
            if(m_src.test(LeftParen)) {
                auto result = makeSizeOf(tok, typeName().get());
                m_src.expect(RightParen);
                return result;
            } else 
                return makeSizeOf(tok, unaryExpr());
        default: 
            m_src.unget(tok); 
            return postfixExpr();
    }
    return makeUnary(tok, op, unaryExpr());
}

/*---------------------------------------------.
|   cast_expression                            |
|       : unary_expression                     |
|       | '(' type_name ')' cast_expression    |
|       ;                                      |
`---------------------------------------------*/
Expr* Parser::castExpr() {
    if(m_src.test(LeftParen)) {
        if(isSpecifier(peek())) {
            auto type = typeName();
            m_src.expect(RightParen);
            return makeCast(castExpr(), type);
        }
    }
    return unaryExpr();
}

/*---------------------------------------------------------.
|   multiplicative_expression                              |
|       : cast_expression                                  |
|       | multiplicative_expression '*' cast_expression    |
|       | multiplicative_expression '/' cast_expression    |
|       | multiplicative_expression '%' cast_expression    |
|       ;                                                  |
`---------------------------------------------------------*/
/*-------------------------------------------------------------.
|   additive_expression                                        |
|       : multiplicative_expression                            |
|       | additive_expression '+' multiplicative_expression    |
|       | additive_expression '-' multiplicative_expression    |
|       ;                                                      |
`-------------------------------------------------------------*/
/*---------------------------------------------------------.
|   shift_expression                                       |
|       : additive_expression                              |
|       | shift_expression LEFT_OP additive_expression     |
|       | shift_expression RIGHT_OP additive_expression    |
|       ;                                                  |
`---------------------------------------------------------*/
/*--------------------------------------------------------.
|   relational_expression                                 |
|       : shift_expression                                |
|       | relational_expression '<' shift_expression      |
|       | relational_expression '>' shift_expression      |
|       | relational_expression LE_OP shift_expression    |
|       | relational_expression GE_OP shift_expression    |
|       ;                                                 |
`--------------------------------------------------------*/
/*-----------------------------------------------------------.
|   equality_expression                                      |
|       : relational_expression                              |
|       | equality_expression EQ_OP relational_expression    |
|       | equality_expression NE_OP relational_expression    |
|       ;                                                    |
`-----------------------------------------------------------*/
/*--------------------------------------------------.
|   and_expression                                  |
|       : equality_expression                       |
|       | and_expression '&' equality_expression    |
|       ;                                           |
`--------------------------------------------------*/
/*------------------------------------------------------.
|   exclusive_or_expression                             |
|       : and_expression                                |
|       | exclusive_or_expression '^' and_expression    |
|       ;                                               |
`------------------------------------------------------*/
/*---------------------------------------------------------------.
|   inclusive_or_expression                                      |
|       : exclusive_or_expression                                |
|       | inclusive_or_expression '|' exclusive_or_expression    |
|       ;                                                        |
`---------------------------------------------------------------*/
/*-----------------------------------------------------------------.
|   logical_and_expression                                         |
|       : inclusive_or_expression                                  |
|       | logical_and_expression AND_OP inclusive_or_expression    |
|       ;                                                          |
`-----------------------------------------------------------------*/
/*--------------------------------------------------------------.
|   logical_or_expression                                       |
|       : logical_and_expression                                |
|       | logical_or_expression OR_OP logical_and_expression    |
|       ;                                                       |
`--------------------------------------------------------------*/
Expr* Parser::binaryExpr() {
    return binaryExpr(castExpr(), 0);
}

Expr* Parser::binaryExpr(Expr *lhs, unsigned preced) {
    auto lop = get(); // operator on the left hand
    auto lprec = precedence(lop->type()); // its precedence
    // while lop is a binary operator and its precedence >= preced
    while(lprec && lprec >= preced) {
        auto rhs = castExpr();
        auto rop = peek(); // operator on the right hand
        auto rprec = precedence(rop->type()); // its precedence
        // while rop is a binary operator and it has a higher precedence
        while(rprec && rprec > lprec) {
            rhs = binaryExpr(rhs, rprec);
            rop = peek();
            rprec = precedence(rop->type());
        }
        lhs = makeBinary(lop, toBinaryOpCode(lop->type()), lhs, rhs);
        lop = get();
        lprec = precedence(lop->type());
    }
    m_src.unget(lop);
    return lhs;
}

/*---------------------------------------------------------------------------.
|   conditional_expression                                                   |
|       : logical_or_expression                                              |
|       | logical_or_expression '?' expression ':' conditional_expression    |
|       ;                                                                    | 
`---------------------------------------------------------------------------*/
Expr* Parser::conditionalExpr() {
    auto result = binaryExpr();
    auto tok = m_src.want(Question);
    if(tok) {
        auto yes = expr();
        m_src.expect(Colon);
        auto no = conditionalExpr();
        return makeTernary(tok, result, yes, no);
    }
    return result;
}

QualType Parser::typeSpecifier(StorageClass *stor) {
    QualType ret{};
    auto tok = get();
    uint32_t qual = 0, spec = 0;
    
    // only one storage class specifier is allowed
    if(isStorageClass(tok->type())) {
        if(!stor) 
            derr << tok->sourceLoc() 
                << "unexpected storage class specifier " << tok->toString();
        
        *stor = toStorageClass(tok->type());
        tok = get();
    }
    
    for(;;tok = get()) {
        markPos(tok); // for error message printing
        auto tokType = tok->type();
        switch(tokType) {
            case KeyConst: case KeyVolatile: case KeyRestrict:
                qual = addQualifier(qual, toQualifier(tokType));
                break;
            case KeyVoid: case KeyChar: case KeyShort:
            case KeyInt: case KeyLong: case KeyFloat:
            case KeyDouble: case KeySigned: case KeyUnsigned:
                spec = addSpecifier(spec, toSpecifier(tokType));
                break;
            case KeyStruct: case KeyUnion: 
                ret.setBase(structUnionSpecifier());
                break;
            case KeyEnum:
                ret.setBase(enumSpecifier());
                break;
            case Identifier: {
                auto id = m_curr->find(tok);
                if(id) {
                    ret = id->type()->clone();
                    break;
                }
            }
            default: goto done;
        }
    }
done:
    if(!spec && !ret) 
        derr << tok->sourceLoc()
            << "unexpected token " << tok->toString();
    m_src.unget(tok);
    if(spec && ret)
        derr << tok->sourceLoc()
            << "multiple data type specification";
    else if(!ret) {
        if(spec == Void) 
            ret = makeVoidType();
        else 
            ret.reset(makeNumberType(spec));
    }
    ret.addQual(qual); // do not override typedef's qualifier
    
    return ret;
}

/*----------------------------------------------------------------------.
|   struct_or_union_specifier                                           |
|       : struct_or_union IDENTIFIER '{' struct_declaration_list '}'    |
|       | struct_or_union '{' struct_declaration_list '}'               |
|       | struct_or_union IDENTIFIER                                    |
|       ;                                                               |
`----------------------------------------------------------------------*/
/*------------------------------------------------------.
|   struct_declaration_list                             |
|       : struct_declaration                            |
|       | struct_declaration_list struct_declaration    |
|       ;                                               |
`------------------------------------------------------*/
/*---------------------------------------------------------------.
|   struct_declaration                                           |
|       : specifier_qualifier_list struct_declarator_list ';'    |
|       ;                                                        |
`---------------------------------------------------------------*/
/*--------------------------------------------------------.
|   struct_declarator_list                                |
|       : struct_declarator                               |
|       | struct_declarator_list ',' struct_declarator    |
|       ;                                                 |
`--------------------------------------------------------*/
/*----------------------------------------------.
|   struct_declarator                           |
|       : declarator                            |
$       | ':' constant_expression               $ // removed for simplicity
$       | declarator ':' constant_expression    $
|       ;                                       |
`----------------------------------------------*/
StructType* Parser::structUnionSpecifier() {
    // KeyStruct/KeyUnion is extracted
    // bool is_struct = tok->m_attr == KeyStruct;
    StructType *type = nullptr; // specifier
    auto tok = get();
    if(tok->is(Identifier)) {
        auto prevTag = m_curr->findTag(tok, false);
        // see '{', means this is a struct/union defintion,
        // override outer scope's struct directly
        if(m_src.test(BlockOpen)) {
            // if the tag is not declared
            if(!prevTag) {
                // declare it
                type = makeStructType();
                m_curr->declareTag(makeDecl(tok, type));
            } else {
                type = prevTag->type()->toStruct();
                if(!type)
                    derr << tok->sourceLoc()
                        << tok->toString() << " is not declared as a struct tag";
            }
            // definition of an incomplete existing tag
            if(type->isComplete())
                // TODO: union check
                derr << tok->sourceLoc()
                    << "redefinition of tag " << tok->toString();
            PtrList members{};
            structDeclList(members);
            type->setMembers(reinterpret_cast<PtrList*>(members.toHeap()));
            m_src.expect(BlockClose);
        } else {
            // without a '{', means this just a type reference
            if(!prevTag) 
                // not found, find it in larger scope
                prevTag = m_curr->findTag(tok);
            if(!prevTag || !prevTag->type()->toStruct()) {
                type = makeStructType();
                m_curr->declareTag(makeDecl(tok, type));
            } else 
                type = prevTag->type()->toStruct();
        }
    } else if(tok->is(BlockOpen)) {
        // anonymous struct tag definition
        PtrList members{};
        structDeclList(members);
        type = makeStructType(reinterpret_cast<PtrList*>(members.toHeap()));
        m_src.expect(BlockClose);
    }
    return type;
}

void Parser::structDeclList(PtrList &members) {
    ScopeGuard guard{m_curr, BlockScope};
    while(isSpecifier(m_src.peek())) {
        auto type = typeSpecifier();
        do {
            auto member = structDeclarator(type);
            members.pushBack(member);
        } while(m_src.test(Comma));
        m_src.expect(Semicolon);
    }
}

Decl* Parser::structDeclarator(QualType base) {
    return declarator(Auto, base);
}

/*----------------------------------------------------------.
|   declaration_specifiers                                  |
|       : storage_class_specifier                           |
|       | storage_class_specifier declaration_specifiers    |
|       | type_specifier                                    |
|       | type_specifier declaration_specifiers             |
|       | type_qualifier                                    |
|       | type_qualifier declaration_specifiers             |
|       ;                                                   |
`----------------------------------------------------------*/
QualType Parser::declSpecifier(StorageClass &stor) {
    return typeSpecifier(&stor);
}

/*--------------------------------------------------------.
|   type_name                                             |
|       : specifier_qualifier_list                        |
|       | specifier_qualifier_list abstract_declarator    |
|       ;                                                 |
`--------------------------------------------------------*/
QualType Parser::typeName() {
    auto type = typeSpecifier();
    switch(m_src.peek()->type()) {
        case Star: case LeftParen: case LeftSubscript: 
            return abstractDeclarator(type);
        default: return type;
    }
}

/*-------------------------------------------.
|   pointer                                  |
|       : '*' pointer                        |
|       | '*' type_qualifier_list pointer    |
|       | EPSILON                            |
|       ;                                    |
`-------------------------------------------*/
QualType Parser::pointer(QualType base) {
    // the first token is ensured to be '*'
    for(;;) {
        auto tok = get();
        markPos(tok);
        if(isQualifier(tok->type()))
            base.addQual(toQualifier(tok->type()));
        else if(tok->is(Star))
            base = QualType{makePointerType(base)};
        else {
            m_src.unget(tok);
            break;
        }
    }
    return base;
}

void Parser::tryDeclarator(QualType &base, Token* &name) {
    base = pointer(base);
    auto tok = get();
    if(tok->is(LeftParen)) {
        // only when no '(' or '[' lies behind the RightParen, 
        // is `base` the type of declarator. So backup first
        auto backup = base;
        tryDeclarator(base, tok);
        m_src.expect(RightParen);
        auto newBase = arrayFuncDeclarator(backup);
        // redirect to the real base type
        for(auto derived = base->toDerived(); derived; ) {
            auto derivedBase = derived->base();
            if(derivedBase != backup)
                derived = derivedBase->toDerived();
            else {
                derived->setBase(newBase);
                break;
            }
        }
    } else {
        if(!tok->is(Identifier)) {
            m_src.unget(tok);
            tok = nullptr;
        }
        base = arrayFuncDeclarator(base);
    }
    name = tok;
}

QualType Parser::arrayFuncDeclarator(QualType base) {
    for(;;) {
        auto tok = get();
        if(tok->is(LeftSubscript)) {// array type
            if(!base->isComplete() || base->toFunc())
                derr << tok->sourceLoc() 
                    << "declaration of array of invalid type";
            int len = 0;
            if(!m_src.test(RightSubscript)) {
                len = evalLong(conditionalExpr());
                m_src.expect(RightSubscript);
            }
            base = makeArrayType(base, len);
        } else if(tok->is(LeftParen)) { // function type
            /* C99 6.7.5.3 Function declarators (including prototypes)
             * 
             * A function declarator shall not specify a return type that is 
             * a function type or an array type.
             */
            if(base->toArray() || base->toFunc()) 
                derr << tok->sourceLoc()
                    << "invalid function return type";
            if(!m_curr->is(FileScope) && !m_curr->is(ProtoScope))
                derr << tok->sourceLoc()
                    << "functions can not be declared here";
            base = paramTypeList(base); // RightParen handled here
        } else {
            m_src.unget(tok);
            break;
        }
    }
    return base;
}

/*----------------------------------------------.
|   abstract_declarator                         |
|       | pointer direct_abstract_declarator    |
|       ;                                       |
`----------------------------------------------*/
/*------------------------------------------------------------------.
|   direct_abstract_declarator                                      |
|       : '(' abstract_declarator ')'                               |
|       | '[' ']'                                                   |
|       | '[' constant_expression ']'                               |
|       | direct_abstract_declarator '[' ']'                        |
|       | direct_abstract_declarator '[' constant_expression ']'    |
|       | '(' ')'                                                   |
|       | '(' parameter_type_list ')'                               |
|       | direct_abstract_declarator '(' ')'                        |
|       | direct_abstract_declarator '(' parameter_type_list ')'    |
|       ;                                                           |
`------------------------------------------------------------------*/
QualType Parser::abstractDeclarator(QualType base) {
    Token *name;
    tryDeclarator(base, name);
    if(name) 
        // warning is better?
        derr << name->sourceLoc()
            << "unexpected identifier";
    return base;
}

/*-------------------------------------.
|   declarator                         |
|       : pointer direct_declarator    | // pointer is nullable
|       ;                              |
`-------------------------------------*/
/*---------------------------------------------------------.
|   direct_declarator                                      |
|       : IDENTIFIER                                       |
|       | '(' declarator ')'                               |
|       | direct_declarator '[' constant_expression ']'    |
|       | direct_declarator '[' ']'                        |
|       | direct_declarator '(' parameter_type_list ')'    |
$       | direct_declarator '(' identifier_list ')'        $ // K&R style function declaration, removed
|       | direct_declarator '(' ')'                        |
|       ;                                                  |
`---------------------------------------------------------*/
Decl* Parser::declarator(StorageClass stor, QualType base) {
    Token *name;
    tryDeclarator(base, name);
    if(!name)
        derr << m_src.peek()->sourceLoc()
            << "expecting an identifier";
    return makeDecl(name, base, stor);
}

/*---------------------------------------.
|   parameter_type_list                  |
|       : parameter_list                 |
|       | parameter_list ',' ELLIPSIS    |
|       ;                                |
`---------------------------------------*/
/*----------------------------------------------------.
|   parameter_list                                    |
|       : parameter_declaration                       |
|       | parameter_list ',' parameter_declaration    |
|       ;                                             |
`----------------------------------------------------*/
/*------------------------------------------------------.
|   parameter_declaration                               |
|       : declaration_specifiers declarator             |
|       | declaration_specifiers abstract_declarator    |
|       | declaration_specifiers                        |
|       ;                                               |
`------------------------------------------------------*/
/* C99 6.7.5.3 Function declarators (including prototypes)
 * 
 * A declaration of a parameter as "array of type" shall be adjusted to "qualified pointer to
 * type", where the type qualifiers (if any) are those specified within the [ and ] of the
 * array type derivation. If the keyword static also appears within the [ and ] of the
 * array type derivation, then for each call to the function, the value of the corresponding
 * actual argument shall provide access to the first element of an array with at least as many
 * elements as specified by the size expression.
 * 
 * A declaration of a parameter as "function returning type" shall be adjusted to "pointer to
 * function returning type", as in 6.3.2.1.
 * 
 * If the list terminates with an ellipsis (, ...), no information about the number or types
 * of the parameters after the comma is supplied.
 * 
 * The special case of an unnamed parameter of type void as the only item in the list
 * specifies that the function has no parameters.
 * 
 * If, in a parameter declaration, an identifier can be treated either as a typedef name or as a
 * parameter name, it shall be taken as a typedef name.
 * 
 * If the function declarator is not part of a definition of that function, parameters may have
 * incomplete type and may use the [*] notation in their sequences of declarator specifiers
 * to specify variable length array types.
 * 
 * The storage-class specifier in the declaration specifiers for a parameter declaration, if
 * present, is ignored unless the declared parameter is one of the members of the parameter
 * type list for a function definition.
 * 
 * For two function types to be compatible, both shall specify compatible return types.
 * 
 * Moreover, the parameter type lists, if both are present, shall agree in the number of
 * parameters and in use of the ellipsis terminator; corresponding parameters shall have
 * compatible types. If one type has a parameter type list and the other type is specified by a
 * function declarator that is not part of a function definition and that contains an empty
 * identifier list, the parameter list shall not have an ellipsis terminator and the type of each
 * parameter shall be compatible with the type that results from the application of the
 * default argument promotions. If one type has a parameter type list and the other type is
 * specified by a function definition that contains a (possibly empty) identifier list, both shall
 * agree in the number of parameters, and the type of each prototype parameter shall be
 * compatible with the type that results from the application of the default argument
 * promotions to the type of the corresponding identifier. (In the determination of type
 * compatibility and of a composite type, each parameter declared with function or array
 * type is taken as having the adjusted type and each parameter declared with qualified type
 * is taken as having the unqualified version of its declared type.)
 */
FuncType* Parser::paramTypeList(QualType ret) {
    // LeftParen handled in caller function
    PtrList params{};
    if(m_src.test(RightParen)) 
        // is a function with an unspecified parameter list
        return makeFuncType(ret, std::move(params), false);
    
    QualType tp;
    Token *name;
    bool vaarg = false;
    
    ScopeGuard guard{m_curr, ProtoScope};
    
    do {
        if(m_src.test(Ellipsis)) {
            vaarg = true; 
            break;
        }
        // no storage class expected
        tp = typeSpecifier();
        markPos(m_src.peek());
        tryDeclarator(tp, name);
        tp = tp.decay();
        // a function with empty parameter list
        if(!name && tp->toVoid() && params.empty()) {
            auto tok = m_src.peek();
            if(!tok->is(RightParen))
                derr << tok->sourceLoc()
                    << "'void' must be the only parameter";
            break;
        } 
        if(!tp->isComplete())
            derr << m_src.peek()->sourceLoc()
                << "parameter declaration with an incomplete type";
        // if name is nullptr means an abstract declarator
        params.pushBack(m_curr->declare(makeDecl(name, tp)));
    } while(m_src.test(Comma));
    
    m_src.expect(RightParen);
    return makeFuncType(ret, std::move(params), vaarg);
}

