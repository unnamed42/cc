#include "text/ustring.hpp"
#include "lexical/token.hpp"
#include "lexical/tokentype.hpp"
#include "semantic/typeenum.hpp"
#include "semantic/type.hpp"
#include "semantic/expr.hpp"
#include "semantic/stmt.hpp"
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
    
    ScopeGuard(Scope* &curr, ScopeType type = BlockScope) :
        curr(curr), new_(type, curr), save(&new_) { std::swap(this->curr, save); }
    ~ScopeGuard() noexcept { std::swap(curr, save); }
};

/**
 * RAII trick to enter new loop and auto exit
 */
struct LoopGuard {
    LabelStmt* &break_;
    LabelStmt* &continue_;
    LabelStmt *nbreak_;
    LabelStmt *ncontinue_;
    
    LoopGuard(LabelStmt* &break_, LabelStmt* &continue_) :
       break_(break_), continue_(continue_), nbreak_(makeLabelStmt()), ncontinue_(makeLabelStmt()) {
           std::swap(this->break_, nbreak_);
           std::swap(this->continue_, ncontinue_);
    }
    
    ~LoopGuard() noexcept {
        std::swap(break_, nbreak_);
        std::swap(continue_, ncontinue_);
    }
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

Parser::Parser(const char *path) : m_src(path) {}

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

void Parser::parse() {
    translationUnit();
}

bool Parser::isSpecifier(Token *name) {
    if(isTypeSpecifier(name->type()))
        return true;
    auto decl = m_curr->find(name);
    return decl && decl->storageClass() == Typedef;
}

ObjectExpr* Parser::makeIdentifier(Token *name) {
    auto decl = m_curr->find(name);
    if(!decl)
        derr.at(name) << "use of undeclared indentifier '" << name << '\'';
    return makeObject(name, decl);
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
            derr.at(tok) << "expecting a primary expression, but get " << tok;
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
        switch(tok->type()) {
            case LeftSubscript:
                result = makeBinary(tok, OpSubscript, result, expr());
                m_src.expect(RightSubscript);
                break;
            case LeftParen: {
                auto resTok = result->token();
                auto func = m_curr->find(resTok);
                auto funcDecl = func ? func->toFuncDecl() : nullptr;
                if(!funcDecl)
                    derr.at(resTok) << "a function designator required";
                result = makeCall(tok, funcDecl, argumentExprList());
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
ExprList Parser::argumentExprList() {
    ExprList list{};
    while(!m_src.nextIs(RightParen)) {
        list.pushBack(assignmentExpr());
        if(!m_src.nextIs(Comma)) {
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
            if(m_src.nextIs(LeftParen)) {
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
    if(m_src.nextIs(LeftParen)) {
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

bool Parser::tryDeclSpecifier(QualType &type, StorageClass *stor, bool required) {
    Token *tok;
    uint32_t qual = 0, spec = 0;
    
    for(;;) {
        tok = get();
        auto tokType = tok->type();
        if(isQualifier(tokType)) 
            qual = addQualifier(qual, tok);
        else if(isTypeSpecifier(tokType)) {
            if(!type.isNull())
                derr.at(tok) << "unexpected token";
            spec = addSpecifier(spec, tok);
        } else if(isStorageClass(tokType)) {
            if(!stor) 
                derr.at(tok) << "unexpected storage class specifier "
                    << static_cast<StorageClass>(tokType);
            *stor = toStorageClass(tok);
        } else if(tokType == KeyEnum) {
            if(!spec) 
                derr.at(tok) << "unexpected token";
            type.setBase(enumSpecifier());
        } else if(tokType == KeyStruct || tokType == KeyUnion) {
            if(!spec) 
                derr.at(tok) << "unexpected token";
            type.setBase(structUnionSpecifier());
        } else if(tokType == Identifier) {
            if(spec || !type.isNull())
                derr.at(tok) << "unexpected token";
            auto id = m_curr->find(tok);
            if(!id || id->isType()) {
                if(required)
                    derr.at(tok) << tok << " does not name a type"; 
                else 
                    break;
            }
            type = id->type()->clone();
        } else
            break;
    }
    
    if(!spec && !type && required) 
        derr.at(tok) << "unexpected token";
    
    m_src.unget(tok);
    if(!type) {
        if(spec == Void) 
            type = makeVoidType();
        else 
            type.reset(makeNumberType(spec));
    }
    type.addQual(qual); // do not override typedef's qualifier
    
    return !type.isNull();
}

QualType Parser::typeSpecifier(StorageClass *stor) {
    QualType ret{};
    tryDeclSpecifier(ret, stor, true);
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
        if(m_src.nextIs(BlockOpen)) {
            // if the tag is not declared
            if(!prevTag) {
                // declare it
                type = makeStructType();
                m_curr->declareTag(makeDecl(tok, type));
            } else {
                type = prevTag->type()->toStruct();
                if(!type)
                    derr.at(tok) << tok << " is not declared as a struct tag";
            }
            // definition of an incomplete existing tag
            if(type->isComplete())
                // TODO: union check
                derr.at(tok) << "redefinition of tag " << tok;
            DeclList members{};
            structDeclList(members);
            type->setMembers(members.toHeap());
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
        DeclList members{};
        structDeclList(members);
        type = makeStructType(members.toHeap());
        m_src.expect(BlockClose);
    }
    return type;
}

void Parser::structDeclList(DeclList &members) {
    ScopeGuard guard{m_curr, BlockScope};
    while(isSpecifier(m_src.peek())) {
        auto type = typeSpecifier();
        do {
            members.pushBack(structDeclarator(type));
        } while(m_src.nextIs(Comma));
        m_src.expect(Semicolon);
    }
}

Decl* Parser::structDeclarator(QualType base) {
    return declarator(Auto, base);
}

/*---------------------------------------------------.
|   enum_specifier                                   |
|       : ENUM '{' enumerator_list '}'               |
|       | ENUM IDENTIFIER '{' enumerator_list '}'    |
|       | ENUM IDENTIFIER                            |
|       ;                                            |
`---------------------------------------------------*/
EnumType* Parser::enumSpecifier() {
    // KeyEnum is recognized
    auto tok = get();
    EnumType *tp = nullptr;
    if(tok->is(Identifier)) {
        auto tag = m_curr->findTag(tok, false);
        auto tagType = tag->type()->toEnum();
        if(tag && tagType)
            tp = tagType;
        else {
            tp = makeEnumType();
            m_curr->declareTag(makeDecl(tok, tp));
        }
        if(m_src.nextIs(BlockOpen)) {
            enumeratorList(); // BlockClose handled here
            tp->setComplete(true);
        }
    } else {
        tp = makeEnumType();
        m_src.expect(BlockOpen);
        enumeratorList(); // BlockClose handled here
        tp->setComplete(true);
    }
    return tp;
}

/*------------------------------------------./+----------------------------------------------.
|   enumerator_list                         ||   enumerator                                  |
|       : enumerator                        ||       : IDENTIFIER                            |
|       | enumerator_list ',' enumerator    ||       | IDENTIFIER '=' constant_expression    |
|       ;                                   ||       ;                                       |
`------------------------------------------+/`----------------------------------------------*/
void Parser::enumeratorList() {
    int curr = 0;
    while(!m_src.nextIs(BlockClose)) {
        auto tok = m_src.want(Identifier);
        if(m_src.nextIs(Assign))
            curr = evalLong(conditionalExpr());
        m_curr->declare(makeDecl(tok, curr++));
        if(!m_src.nextIs(Comma)) {
            m_src.expect(BlockClose);
            return;
        }
    }
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
        if(isQualifier(tok->type()))
            base.addQual(toQualifier(tok));
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
                derr.at(tok) << "declaration of array of invalid type " << base;
            int len = 0;
            if(!m_src.nextIs(RightSubscript)) {
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
                derr.at(tok) << "invalid function return type";
            if(!m_curr->is(FileScope) && !m_curr->is(ProtoScope))
                derr.at(tok) << "functions can not be declared here";
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
        derr.at(name) << "unexpected identifier";
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
        derr.at(peek()) << "expecting an identifier";
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
    DeclList params{};
    if(m_src.nextIs(RightParen)) 
        // is a function with an unspecified parameter list
        return makeFuncType(ret, std::move(params), false);
    
    QualType tp;
    Token *name;
    bool vaarg = false;
    
    ScopeGuard guard{m_curr, ProtoScope};
    
    do {
        if(m_src.nextIs(Ellipsis)) {
            vaarg = true; 
            break;
        }
        // no storage class expected
        tp = typeSpecifier();
        tryDeclarator(tp, name);
        tp = tp.decay();
        // a function with empty parameter list
        if(!name && tp->toVoid() && params.empty()) {
            auto tok = m_src.peek();
            if(!tok->is(RightParen))
                derr.at(tok) << "'void' must be the only parameter";
            break;
        } 
        if(!tp->isComplete())
            derr.at(peek()) << "parameter declaration with an incomplete type";
        // if name is nullptr means an abstract declarator
        params.pushBack(m_curr->declare(makeDecl(name, tp)));
    } while(m_src.nextIs(Comma));
    
    m_src.expect(RightParen);
    return makeFuncType(ret, std::move(params), vaarg);
}

/*-----------------------------------------------------------.
|   declaration                                              |
|       : declaration_specifiers ';'                         |
|       | declaration_specifiers init_declarator_list ';'    |
|       ;                                                    |
`-----------------------------------------------------------*/
void Parser::declaration(StmtList &list, QualType type, StorageClass stor) {
    auto tok = m_src.want(Semicolon);
    if(tok) {
        if(!type->toStruct() && !type->toEnum())
            derr.at(tok) << "declaration does not declare anything";
    } else {
        initDeclarators(list, type, stor);
        m_src.expect(Semicolon);
    }
}

/*----------------------------------------------------./+--------------------------------------.
|   init_declarator_list                              ||   init_declarator                     |
|       : init_declarator                             ||       : declarator                    |
|       | init_declarator_list ',' init_declarator    ||       | declarator '=' initializer    |
|       ;                                             ||       ;                               |
`----------------------------------------------------+/`--------------------------------------*/
void Parser::initDeclarators(Utils::StmtList &list, QualType type, StorageClass stor) {
    //init_list inits{};
    do {
        auto newType = type;
        Token *name;
        Expr *init = nullptr;
        tryDeclarator(newType, name);
        if(!name) 
            derr.at(peek()) << "expecting an identifier";
        if(m_src.nextIs(Assign)) 
            init = initializer(type);
        
        auto &&decl = m_curr->declare(makeDecl(name, newType, stor));
        decl->setInit(init);
        list.pushBack(decl);
    } while(m_src.nextIs(Comma));
}

/*----------------------------------------./+--------------------------------------------.
|   initializer                           ||   initializer_list                          |
|       : assignment_expression           ||       : initializer                         |
|       | '{' initializer_list '}'        ||       | initializer_list ',' initializer    |
|       | '{' initializer_list ',' '}'    ||       ;                                     |
|       ;                                 |`--------------------------------------------+/
`----------------------------------------*/
Expr* Parser::initializer(QualType type) {
    auto tok = get();
    if(tok->is(BlockOpen)) {
        if(type->toArray())
            return arrayInitializer(type->toArray());
        else if(type->toStruct())
            return aggregateInitializer(type->toStruct());
        else 
            derr.at(tok) << "expecting an aggregate type";
    } else if(tok->is(String) && type->toArray()) {
        auto arrType = type->toArray();
        auto elemType = arrType->base();
        
        auto &str = *tok->content();
        if(!elemType->toNumber() && !elemType->toNumber()->isChar())
            derr.at(tok) << "cannot initialize type" << type << "with string literal";
        if(arrType->isComplete()) {
            if(arrType->bound() <= str.dataLength())
                derr.at(tok) << "string is too long";
        } else
            arrType->setBound(str.dataLength() + 1); // one extra for '\0'
        
        return nullptr; // TODO: return expression here
    } else {
        m_src.unget(tok);
        return assignmentExpr();
    }
}

Expr* Parser::arrayInitializer(ArrayType *type) {
    unsigned index = 0, len = type->bound();
    auto base = type->base();
    
    while(!m_src.nextIs(BlockClose)) {
        if(m_src.nextIs(LeftSubscript)) {
            auto offset = conditionalExpr();
            auto index_ = evalLong(offset);
            if(index_ < index) 
                derr.at(offset->token()) << "invalid offset expression";
            // TODO: insert elements
            m_src.expect(RightSubscript);
            m_src.expect(Assign);
        }
        initializer(base);
        ++index;
        if(!m_src.nextIs(Comma)) {
            m_src.expect(BlockClose);
            break;
        }
    }
    
    if(!type->isComplete())
        type->setBound(index);
    else if(len < index)
        derr.at(peek()) << "excess element number";
    
    // TODO: return expression here
}

Expr* Parser::aggregateInitializer(StructType *type) {
    if(!type->isComplete())
        derr.at(peek()) << "initializer for incomplete struct";
    
    auto &&members = type->members();
    auto it = members.begin();
    while(!m_src.nextIs(BlockClose))
        initializer((*it)->type()); // TODO: 
    // TODO: return expression here
}

/*--------------------------------.
|   statement                     |
|       : labeled_statement       |
|       | compound_statement      |
|       | expression_statement    |
|       | selection_statement     |
|       | iteration_statement     |
|       | jump_statement          |
|       ;                         |
`--------------------------------*/
Stmt* Parser::statement() {
    auto tok = get();
    switch(tok->type()) {
//        case KeyCase: case KeyDefault:
//            return label_stmt();
        case Semicolon: return makeStmt();
        case BlockOpen: return compoundStatement();
        case KeyIf: /*case KeySwitch: */
            return selectionStatement();
        case KeyFor: return forLoop();
        case KeyDo:  return doWhileLoop();
        case KeyWhile: return whileLoop();
        case KeyGoto: case KeyReturn: 
        case KeyContinue: case KeyBreak:
            m_src.unget(tok);
            return jumpStatement();
        case Identifier: 
            if(m_src.peek(Colon)) {
                m_src.unget(tok);
                return labelStatement();
            }
        default: {
            m_src.unget(tok);
            auto res = expr();
            m_src.expect(Semicolon);
            return res;
        }
    }
}

/*--------------------------------------------------.
|   labeled_statement                               |
|       : IDENTIFIER ':' statement                  |
|       | CASE constant_expression ':' statement    | // TODO:
|       | DEFAULT ':' statement                     | // TODO
|       ;                                           |
`--------------------------------------------------*/
CompoundStmt* Parser::labelStatement() {
    auto peek = this->peek();
//     StmtList l{};
//     if(peek->m_attr == Identifier) {
//         m_src.ignore();
//         m_src.expect(Colon);
//         auto dest = statement();
//         auto name = peek->to_string();
//         auto label = m_lmap.find(name);
//         if(label != m_lmap.end())
//             error(peek, "Redefinition of label \"%s\"", name);
//         else 
//             label = m_lmap.insert({name, make_label()}).first;
//         l.pushBack(label->second);
//         l.pushBack(dest);
//     }
//     return make_compound(m_curr, std::move(l));
    return nullptr;
}

/*---------------------------------------------------.
|   compound_statement                               |
|       : '{' '}'                                    |
|       | '{' statement_list '}'                     |
|       | '{' declaration_list '}'                   |
|       | '{' declaration_list statement_list '}'    |
|       ;                                            |
`---------------------------------------------------*/
/*----------------------------------------./+------------------------------------.
|   declaration_list                      ||   statement_list                    |
|       : declaration                     ||       : statement                   |
|       | declaration_list declaration    ||       | statement_list statement    |
|       ;                                 ||       ;                             |
`----------------------------------------+/`------------------------------------*/
CompoundStmt* Parser::compoundStatement(QualType func) {
//    m_src.expect(BlockOpen); // extracted in caller function
    ScopeGuard guard{m_curr};
    
    if(func) {
        // insert parameters
        for(auto param : func->toFunc()->params())
            m_curr->declare(param);
    }
    
    StmtList l{};
    while(!m_src.nextIs(BlockClose)) {
        QualType decl{};
        StorageClass stor = Auto;
        if(tryDeclSpecifier(decl, &stor))
            declaration(l, decl, stor);
        else
            l.pushBack(statement());
    }
    return makeCompoundStmt(std::move(l));
}

/*----------------------------------------------------------.
|   selection_statement                                     |
|       : IF '(' expression ')' statement                   |
|       | IF '(' expression ')' statement ELSE statement    |
|       | SWITCH '(' expression ')' statement               | // TODO
|       ;                                                   |
`----------------------------------------------------------*/
CondStmt* Parser::selectionStatement() {
    //m_src.expect(If); // extracted in caller function
    m_src.expect(LeftParen);
    auto cond = expr();
    m_src.expect(RightParen);
    auto yes = statement();
    auto no = m_src.nextIs(KeyElse) ? statement() : nullptr;
    return makeCondStmt(cond, yes, no);
}

/*--------------------------------------------------------------------------------------.
|   iteration_statement                                                                 |
|       : WHILE '(' expression ')' statement                                            |
|       | DO statement WHILE '(' expression ')' ';'                                     |
|       | FOR '(' expression_statement expression_statement ')' statement               |
|       | FOR '(' expression_statement expression_statement expression ')' statement    |
|       ;                                                                               |
`--------------------------------------------------------------------------------------*/
CompoundStmt* Parser::whileLoop() {
    //m_src.expect(KeyWhile); extracted in caller function
    m_src.expect(LeftParen);
    
    StmtList l{};
    
    LoopGuard  _{m_break, m_continue};
    ScopeGuard __{m_curr};
    
    auto cond = expr();
    m_src.expect(RightParen);
    
    auto body = statement();
    auto bodyLabel = makeLabelStmt();
    auto condStmt = makeCondStmt(cond, makeJumpStmt(bodyLabel), makeJumpStmt(m_break));
    auto loop = makeJumpStmt(m_continue);
    
    l.pushBack(m_continue);
    l.pushBack(condStmt);
    l.pushBack(bodyLabel);
    l.pushBack(body);
    l.pushBack(loop);
    l.pushBack(m_break);
    
    return makeCompoundStmt(std::move(l));
}

CompoundStmt* Parser::doWhileLoop() {
    // m_src.expect(KeyDo); extracted in caller function
    StmtList l{};
    
    LoopGuard  _{m_break, m_continue};
    ScopeGuard __{m_curr};
    
    auto body = statement();
    m_src.expect(KeyWhile);
    m_src.expect(LeftParen);
    auto cond = expr();
    m_src.expect(RightParen);
    m_src.expect(Semicolon);
    
    auto condStmt = makeCondStmt(cond, makeJumpStmt(m_continue), makeJumpStmt(m_break));
    
    l.pushBack(m_continue);
    l.pushBack(body);
    l.pushBack(condStmt);
    l.pushBack(m_break);
    
    return makeCompoundStmt(std::move(l));
}

CompoundStmt* Parser::forLoop() {
    // m_src.expect(KeyFor); extracted in caller function
    m_src.expect(LeftParen);
    StmtList l{};
    
    LoopGuard  _{m_break, m_continue};
    ScopeGuard __{m_curr};
    
    QualType type{}; StorageClass stor = Auto;
    if(tryDeclSpecifier(type, &stor))
        declaration(l, type, stor);
    else if(!m_src.nextIs(Semicolon)) 
        l.pushBack(expr());
    
    Expr *cond = nullptr;
    if(!m_src.nextIs(Semicolon)) {
        cond = expr();
        m_src.expect(Semicolon);
    } else
        cond = makeInteger(1);
    
    Stmt *step = nullptr;
    if(!m_src.nextIs(RightParen)) {
        step = expr();
        m_src.expect(RightParen);
    } else 
        step = makeStmt();
    
    auto body = statement();
    auto bodyLabel = makeLabelStmt();
    auto condLabel = makeLabelStmt();
    auto condStmt = makeCondStmt(cond, makeJumpStmt(bodyLabel), makeJumpStmt(m_break));
    auto loop = makeJumpStmt(condLabel);
    
    l.pushBack(condLabel);
    l.pushBack(condStmt);
    l.pushBack(bodyLabel);
    l.pushBack(body);
    l.pushBack(m_continue);
    l.pushBack(step);
    l.pushBack(loop);
    l.pushBack(m_break);
    
    return makeCompoundStmt(std::move(l));
}

/*---------------------------------.
|   jump_statement                 |
|       : GOTO IDENTIFIER ';'      |
|       | CONTINUE ';'             |
|       | BREAK ';'                |
|       | RETURN ';'               |
|       | RETURN expression ';'    |
|       ;                          |
`---------------------------------*/
JumpStmt* Parser::jumpStatement() {
    auto tok = get();
    JumpStmt *res = nullptr;
    switch(tok->type()) {
        case KeyGoto: {
            tok = m_src.want(Identifier);
            auto name = tok->content();
            auto label = m_lmap.find(name);
            auto jump = make_jump(nullptr);
            if(label == m_lmap.end()) 
                m_unresolved.pushBack({tok, jump});
            else
                jump->label = label->second;
            res = jump;
            break;
        }
        case KeyContinue:
            if(!m_continue)
                derr.at(tok) << "use \"continue\" out of loop";
            res = makeJumpStmt(m_continue);
            break;
        case KeyBreak:
            if(!m_break)
                derr.at(tok) << "use \"break\" out of loop";
            res = makeJumpStmt(m_break);
            break;
        case KeyReturn:
            if(!m_func)
                derr.at(tok) << "Use \"return\" out of function";
            if(m_src.peek(Semicolon))
                res = make_return(m_func);
            else 
                res = make_return(m_func, expr());
        default: break;
    }
    m_src.expect(Semicolon);
    return res;
}

/*-------------------------------------------------./+-------------------------------.
|   translation_unit                               ||   external_declaration         |
|       : external_declaration                     ||       : function_definition    |
|       | translation_unit external_declaration    ||       | declaration            |
|       ;                                          ||       ;                        |
`-------------------------------------------------+/`-------------------------------*/
void Parser::translationUnit() {
    while(!m_src.nextIs(Eof)) {
        if(m_src.nextIs(Semicolon))
            continue;
        
        StorageClass stor;
        auto base = declSpecifier(stor);
        
        auto etok = peek(); // token for diagnostic use
        if(m_src.nextIs(Semicolon)) {
            if((base->toStruct() || base->toEnum()) && !stor)
                continue;
            derr.at(etok) << "expecting an identifier name";
        }
        
        etok = peek();
        
        auto declType = base;
        Token *name;
        tryDeclarator(declType, name);
        if(!name)
            derr.at(etok) << "unexpected abstract declarator";
        
        if(declType->toFunc()) {
            if(m_src.nextIs(BlockOpen)) 
                ;//m_tu.pushBack(function_definition(name, declType, stor));
            else {
                //m_tu.pushBack(m_curr->declare_func(name, declType, stor, nullptr)->decl);
                m_src.expect(Semicolon);
            }
        } else {
//             init_list inits{};
//             if(m_src.nextIs(Assign)) 
//                 inits = initializer(declType);
//             auto var_decl = m_curr->declare(name, declType, stor)->decl;
//             var_decl->inits = std::move(inits);
//             if(m_src.nextIs(Comma))
//                 init_declarators(m_tu, stor, base);
            m_src.expect(Semicolon);
        }
    }
}

/*---------------------------------------------------------------------------------.
|   function_definition                                                            |
$       : declaration_specifiers declarator declaration_list compound_statement    $ // K&R
|       | declaration_specifiers declarator compound_statement                     |
$       | declarator declaration_list compound_statement                           $ // K&R
$       | declarator compound_statement                                            $ // K&R
|       ;                                                                          |
`---------------------------------------------------------------------------------*/
DeclStmt* Parser::functionDefinition(Token *name, QualType func, uint32_t stor) {
    auto id = m_curr->find(name);
    if(id) {
        auto ptype = id->type(); // previous declared type
        if(!ptype->toFunc())
            derr.at(id) << name << "is not declared as function";
        
        auto pfunc = reinterpret_cast<FuncDecl*>(id);
        if(pfunc->body())
            derr.at(name) << name << "already has a definition";
        if(!ptype->isCompatible(func))
            derr.at(name) << "mismatched function signature";
//        m_func = pfunc;
        // prototype may have anonymous parameter, update it
        pfunc->updateSignature(func);
        pfunc->setBody(compoundStatement(func));
//         for(auto &&resolve: m_unresolved) {
//             auto lname = resolve.first->to_string();
//             auto it = m_lmap.find(lname);
//             if(it == m_lmap.end())
//                 error(resolve.first, "Unresolved label \"%s\"", lname);
//             resolve.second->label = it->second;
//         }
//         m_lmap.clear();
//         m_unresolved.clear();
//         m_func = nullptr;
//        return id;
        return nullptr;
//     } else {
//         auto func = m_curr->declare_func(name, tp, stor);
//         m_func = func;
//         func->body = compound_stmt(tp);
//         for(auto &&resolve: m_unresolved) {
//             auto lname = resolve.first->to_string();
//             auto it = m_lmap.find(lname);
//             if(it == m_lmap.end())
//                 error(resolve.first, "Unresolved label \"%s\"", lname);
//             resolve.second->label = it->second;
//         }
//         m_lmap.clear();
//         m_unresolved.clear();
//         m_func = nullptr;
//         return func->decl;
    }
}
