#include "ast.hpp"
#include "type.hpp"
#include "error.hpp"
#include "parser.hpp"
#include "evaluator.hpp"

#include <list>
#include <cctype>
#include <cstdint>

using namespace compiler;

static constexpr unsigned int min_prec = 0;

static unsigned int precedence(token*);

static bool specifier_peek(token*, scope*);
static bool decl_peek(token*, scope*);


parser::parser()
    :m_cpp(), m_file(make_scope(nullptr, FILE_SCOPE)), m_curr(m_file), m_break(nullptr), m_continue(nullptr), m_tu(), m_func(nullptr), m_lmap(), m_unresolved() {}

parser::parser(const char *file)
    :m_cpp(file), m_file(make_scope(nullptr, FILE_SCOPE)), m_curr(m_file), m_break(nullptr), m_continue(nullptr), m_tu(), m_func(nullptr), m_lmap(), m_unresolved() {}

ast_ident* parser::make_identifier(token *tok) {
    auto id = m_curr->find(tok);
    if(!id)
        error(tok, "Use of undeclared identifier \"%s\"", tok->to_string());
    return id;
}

ast_ident* parser::get_identifier() {
    auto tok = m_cpp.get();
    if(tok->m_attr == Identifier)
        return make_identifier(tok);
    error(tok, "Expecting an identifier, but get \"%s\"", tok->to_string());
    return nullptr;
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
ast_expr* parser::primary_expr() {
    auto tok = m_cpp.get();
    switch(tok->m_attr) {
        case Identifier: return make_identifier(tok);
        case String: return make_string(tok);
        case Char: return make_char(tok); //
        case PPNumber: case PPFloat: return make_number(tok);
        case LeftParen: {
            auto res = expr(); m_cpp.expect(RightParen); return res;
        }
        case KeyTrue: case KeyFalse: return make_bool(tok);
        default: 
            error(tok, "Expecting a primary expression, but get %s", tok->to_string());
            return nullptr;
    }
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
ast_expr* parser::postfix_expr() {
    auto result = primary_expr();
    for(;;) {
        auto tok = m_cpp.get();
        mark_pos(tok);
        switch(tok->m_attr) {
            case LeftSubscript:
                result = make_binary(tok, result, expr(), Subscript);
                m_cpp.expect(RightSubscript);
                break;
            case LeftParen: {
                auto res_tok = result->m_tok;
                ast_func *func = nullptr;
                if(res_tok->is(Identifier)) {
                    func = m_curr->find(res_tok)->to_func();
                    if(!func)
                        error(res_tok, "A function designator required");
                } else 
                    error(res_tok, "A function designator required");
                result = make_call(tok, func, argument_expr_list());
                //m_cpp.expect(RightParen); handled in args
                break;
            }
            case Inc: result = make_unary(tok, result, PostInc); break;
            case Dec: result = make_unary(tok, result, PostDec); break;
            case Dot: case MemberPtr: 
                // symbol resolution is handled in make function
                result = make_member_access(tok, result, m_cpp.get(Identifier));
                break;
            default: m_cpp.unget(tok); return result;
        }
    }
}

/*------------------------------------------------.
|   expression                                    |
|       : assignment_expression                   |
|       | expression ',' assignment_expression    |
|       ;                                         |
`------------------------------------------------*/
ast_expr* parser::expr() {
    auto result = assignment_expr();
    while(m_cpp.test(Comma))
        result = make_binary(nullptr, result, assignment_expr(), Comma);
    return result;
}

// same definition as expression
arg_list parser::argument_expr_list() {
    arg_list l{};
    while(!m_cpp.test(RightParen)) {
        l.push_back(assignment_expr());
        if(!m_cpp.test(Comma)) {
            m_cpp.expect(RightParen);
            break;
        }
    }
    return l;
}

/*---------------------------------------------------------------------------.
|   assignment_expression                                                    |
|       : conditional_expression                                             |
$       | unary_expression ASSIGNMENT_OPERATOR assignment_expression         $ // C99 standard
|       | logical_or_expression ASSIGNMENT_OPERATOR assignment_expression    | // simplified version
|       ;                                                                    |
`---------------------------------------------------------------------------*/
ast_expr* parser::assignment_expr() {
    auto result = binary_expr();
    auto tok = m_cpp.get();
    auto &&attr = tok->m_attr;
    
    if(attr == Question) {
        auto _true = expr();
        m_cpp.expect(Colon);
        auto _false = conditional_expr();
        return make_ternary(result, _true, _false);
    } else if(is_assignment(attr)) 
        return make_assignment(tok, result, assignment_expr(), attr);
    else {
        m_cpp.unget(tok);
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
ast_expr* parser::unary_expr() {
    uint32_t op;
    auto tok = m_cpp.get();
    switch(tok->m_attr) {
        case Dec: case Inc: op = tok->m_attr; break;
        case Ampersand: op = AddressOf; break;
        case Star: op = Dereference; break;
        case Add: op = ArithmeticOf; break;
        case Sub: op = Negate; break;
        case KeySizeof: 
            if(m_cpp.test(LeftParen)) {
                auto result = make_sizeof(tok, type_name());
                m_cpp.expect(RightParen);
                return result;
            } else return make_sizeof(tok, unary_expr());
        default: m_cpp.unget(tok); return postfix_expr();
    }
    return make_unary(tok, unary_expr(), op);
}

/*---------------------------------------------.
|   cast_expression                            |
|       : unary_expression                     |
|       | '(' type_name ')' cast_expression    |
|       ;                                      |
`---------------------------------------------*/
ast_expr* parser::cast_expr() {
    auto paren = m_cpp.peek();
    if(paren->is(LeftParen)) {
        m_cpp.ignore();
        auto tok = m_cpp.peek();
        if(specifier_peek(tok, m_curr)) {
            auto tp = type_name();
            m_cpp.expect(RightParen);
            return make_cast(tok, tp, cast_expr());
        } else 
            m_cpp.unget(paren);
    } 
    return unary_expr();
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
ast_expr* parser::binary_expr() {
    return binary_expr(cast_expr(), min_prec);
}

ast_expr* parser::binary_expr(ast_expr *lhs, unsigned int preced) {
    auto lop = m_cpp.get(); // operator on the left hand
    auto lprec = precedence(lop); // its precedence
    // while lop is a binary operator and its precedence >= preced
    while(lprec && lprec >= preced) {
        uint32_t op;
        // translate opcode
        switch(lop->m_attr) {
            case Star: op = Mul; break;
            case Ampersand: op = BitAnd; break;
            default: op = lop->m_attr;
        }
        auto rhs = cast_expr();
        auto rop = m_cpp.peek(); // operator on the right hand
        auto rprec = precedence(rop); // its precedence
        // while rop is a binary operator and it has a higher precedence
        while(rprec && rprec > lprec) {
            rhs = binary_expr(rhs, rprec);
            rop = m_cpp.peek();
            rprec = precedence(rop);
        }
        lhs = make_binary(lop, lhs, rhs, op);
        lop = m_cpp.get();
        lprec = precedence(lop);
    }
    m_cpp.unget(lop);
    return lhs;
}

/*---------------------------------------------------------------------------.
|   conditional_expression                                                   |
|       : logical_or_expression                                              |
|       | logical_or_expression '?' expression ':' conditional_expression    |
|       ;                                                                    | 
`---------------------------------------------------------------------------*/
ast_expr* parser::conditional_expr() {
    auto result = binary_expr();
    if(m_cpp.test(Question)) {
        auto _true = expr();
        m_cpp.expect(Colon);
        auto _false = conditional_expr();
        return make_ternary(result, _true, _false);
    }
    return result;
}


// TODO:
/*-----------------------------------------------------------.
|   declaration                                              |
|       : declaration_specifiers ';'                         |
|       | declaration_specifiers init_declarator_list ';'    |
|       ;                                                    |
`-----------------------------------------------------------*/
void parser::decl(stmt_list &l) {
    uint8_t stor = 0;
    auto tp = decl_specifiers(stor); // type
    auto tok = m_cpp.peek();
    if(tok->is(Semicolon)) {
        if(!tp->is_struct() && !tp->is_union() && !tp->is_enum())
            error(tok, "Declaration does not declare anything");
        m_cpp.ignore();
    } else {
        init_declarators(l, stor, tp);
        m_cpp.expect(Semicolon);
    }
}

/*----------------------------------------------------./+--------------------------------------.
|   init_declarator_list                              ||   init_declarator                     |
|       : init_declarator                             ||       : declarator                    |
|       | init_declarator_list ',' init_declarator    ||       | declarator '=' initializer    |
|       ;                                             ||       ;                               |
`----------------------------------------------------+/`--------------------------------------*/
void parser::init_declarators(stmt_list &l, uint8_t stor, qual_type tp) {
    init_list inits{};
    do {
        auto new_type = tp;
        auto name = try_declarator(new_type);
        if(!name) 
            error(m_cpp.peek(), "Expecting an identifier");
        if(m_cpp.test(Assign)) 
            inits = initializer(tp);
        auto &&decl = m_curr->declare(name, new_type, stor)->decl;
        decl->inits = std::move(inits);
        l.push_back(decl);
    } while(m_cpp.test(Comma));
}

qual_type parser::type_specifier(uint8_t *stor) {
    qual_type res = qual_null;
    token *tok = m_cpp.get();
    uint8_t qual = 0; uint32_t spec = 0;
    
    // only one storage class specifier is allowed
    if(is_storage_specifier(tok->m_attr)) {
        if(!stor) error(tok, "Unexpected storage class specifier \"%s\"", tok->to_string());
        *stor = attr_to_spec(tok->m_attr);
        tok = m_cpp.get();
    }
    
    for(;;tok = m_cpp.get()) {
        auto &&attr = tok->m_attr;
        auto converted = attr_to_spec(attr);
        if(converted == Error) break;
        mark_pos(tok); // for error message printing
        switch(attr) {
            case KeyConst: case KeyVolatile: case KeyRestrict:
                qual = apply_qual(qual, converted);
                break;
            case KeyVoid: case KeyChar: case KeyShort:
            case KeyInt: case KeyLong: case KeyFloat:
            case KeyDouble: case KeySigned: case KeyUnsigned:
                spec = apply_spec(spec, converted);
                break;
            case KeyStruct: case KeyUnion: 
                res.set_base(struct_union_specifier());
                break;
            case KeyEnum:
                res.set_base(enum_specifier());
                break;
            case Identifier: {
                auto id = m_curr->find(tok);
                if(id && id->to_type()) {
                    res = id->to_type().copy();
                    break;
                }
            }
            default: goto out_loop;
        }
    }
out_loop:
    if(!spec && !res) 
        error(tok, "Unexpected token %s", tok->to_string());
    m_cpp.unget(tok);
    if(spec && res)
        error(tok, "Multiple data type specification");
    else if(!res) {
        if(spec & Void) 
            res = make_void();
        else 
            res.reset(make_arith(spec));
    }
    res.add_qual(qual); // do not override typedef's qualifier
    
    return res;
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
qual_type parser::decl_specifiers(uint8_t &stor) {
    return type_specifier(&stor);
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
type_struct* parser::struct_union_specifier() {
    // KeyStruct/KeyUnion is recognized
    // bool is_struct = tok->m_attr == KeyStruct;
    type_struct *spec = nullptr; // specifier
    auto tok = m_cpp.get();
    
    if(tok->m_attr == Identifier) {
        auto prev_tag = m_curr->find_tag_current(tok);
        // see '{', means this is a struct/union defintion,
        // override outer scope's struct directly
        if(m_cpp.test(BlockOpen)) {
            // if the tag is not declared
            if(!prev_tag) {
                // declare it
                spec = make_struct();
                m_curr->declare_tag(tok, spec);
            } else {
                spec = prev_tag->m_type->to_struct();
                if(!spec) 
                    error(tok, "\"%s\" is not declared as a struct tag", tok->to_string());
            }
            // definition of an incomplete existing tag
            if(spec->is_complete())
                // TODO: union check
                error(tok, "Redefinition of tag \"%s\"", tok->to_string());
            member_list mem{};
            spec->set_scope(struct_decl_list(mem));
            spec->set_members(std::move(mem));
            m_cpp.expect(BlockClose);
        } else {
            // without a '{', means this just a type reference
            if(!prev_tag) 
                // not found, find it in larger scope
                prev_tag = m_curr->find_tag(tok);
            if(!prev_tag || !prev_tag->m_type->is_struct()) {
                spec = make_struct();
                m_curr->declare_tag(tok, spec);
            } else 
                spec = prev_tag->m_type->to_struct();
        }
    } else if(tok->is(BlockOpen)) {
        // anonymous struct tag definition
        member_list mem{};
        auto s = struct_decl_list(mem);
        spec = make_struct(s, std::move(mem));
        m_cpp.expect(BlockClose);
    }
    return spec;
}

// TODO: allow empty list?
scope* parser::struct_decl_list(member_list &m) {
    auto s = make_scope(m_curr);
    std::swap(s, m_curr);
    while(specifier_peek(m_cpp.peek(), m_curr)) {
        auto tp = type_specifier(nullptr); // specifier-qualifier-list
        do {
            auto member = struct_declarator(tp);
            m.push_back(member);
        } while(m_cpp.test(Comma)); // struct-declarator-list
        m_cpp.expect(Semicolon);
    }
    std::swap(s, m_curr);
    return s;
}

ast_object* parser::struct_declarator(qual_type tp) {
    return declarator(0, tp);
}

/*--------------------------------------------------------.
|   type_name                                             |
|       : specifier_qualifier_list                        |
|       | specifier_qualifier_list abstract_declarator    |
|       ;                                                 |
`--------------------------------------------------------*/
qual_type parser::type_name() {
    auto tp = type_specifier(nullptr);
    switch(m_cpp.peek()->m_attr) {
        case Star: case LeftParen: case LeftSubscript: 
            return abstract_declarator(tp);
        default: return tp;
    }
}

/*-------------------------------------------.
|   pointer                                  |
|       : '*' pointer                        |
|       | '*' type_qualifier_list pointer    |
|       | EPSILON                            |
|       ;                                    |
`-------------------------------------------*/
// nullable grammar variable pointer
qual_type parser::pointer(qual_type base) {
    // the first token is ensured to be '*'
    for(;;) {
        auto tok = m_cpp.get();
        mark_pos(tok);
        if(is_type_qualifier(tok->m_attr))
            base.add_qual(attr_to_spec(tok->m_attr));
        else if(tok->is(Star))
            base = qual_pointer(base);
        else {
            m_cpp.unget(tok);
            break;
        }
    }
    return base;
}

token* parser::try_declarator(qual_type &base) {
    base = pointer(base);
    token *tok = m_cpp.get();
    if(tok->is(LeftParen)) {
        // only when no '(' or '[' lies behind the RightParen, 
        // is `base` the type of declarator. So backup first
        auto backup = base;
        tok = try_declarator(base); // name of declared item
        m_cpp.expect(RightParen);
        auto new_base = arr_func_declarator(backup);
        // redirect to the real base type
        for(auto derived = base->to_derived(); derived; ) {
            if(derived->get() != backup) 
                derived = derived->to_derived();
            else {
                derived->set_base(new_base); break;
            }
        }
    } else {
        if(!tok->is(Identifier)) {
            m_cpp.unget(tok); 
            tok = nullptr;
        }
        base = arr_func_declarator(base);
    }
    return tok;
}

qual_type parser::arr_func_declarator(qual_type base) {
    for(;;) {
        auto tok = m_cpp.get();
        if(tok->is(LeftSubscript)) {// array type
            if(!base->is_complete() || base->is_func())
                error(tok, "Declaration of array of invalid type");
            long len = 0;
            if(!m_cpp.test(RightSubscript)) {
                len = eval_long(conditional_expr());
                m_cpp.expect(RightSubscript);
            }
            base = make_array(base, len);
        } else if(tok->is(LeftParen)) {// function type
            /* C99 6.7.5.3 Function declarators (including prototypes)
             * 
             * A function declarator shall not specify a return type that is 
             * a function type or an array type.
             */
            if(base->is_array() || base->is_func()) 
                error(tok, "Invalid function return type");
            if(m_curr->kind() != FILE_SCOPE && m_curr->kind() != PROTO_SCOPE) 
                error(tok, "Functions can not be declared here");
            base = param_type_list(base); // RightParen handled here
        } else {
            m_cpp.unget(tok);
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
qual_type parser::abstract_declarator(qual_type tp) {
    auto name = try_declarator(tp);
    if(name) 
        // warning is better?
        error(name, "Unexpected identifier");
    return tp;
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
ast_object* parser::declarator(uint8_t stor, qual_type tp) {
    auto name = try_declarator(tp);
    if(!name) 
        error(m_cpp.peek(), "Expecting an identifier");
    return m_curr->declare(name, tp, stor);
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
qual_type parser::param_type_list(qual_type ret) {
    // LeftParen handled in caller function
    param_list params{};
    if(m_cpp.test(RightParen)) 
        // is a function with an unspecified parameter list
        return make_func(ret, std::move(params), false, true);
    
    qual_type tp; 
    bool vaarg = false;
    auto s = make_scope(m_curr, PROTO_SCOPE);
    std::swap(s, m_curr);
    do {
        if(m_cpp.test(Ellipsis)) {
            vaarg = true; break;
        }
        // no storage class expected
        tp = type_specifier(nullptr);
        mark_pos(m_cpp.peek());
        auto name = try_declarator(tp);
        tp = tp.decay();
        // a function with empty parameter list
        if(!name && tp->is_void() && params.empty()) {
            auto tok = m_cpp.peek();
            if(!tok->is(RightParen))
                error(tok, "\"void\" must be the only parameter");
            break;
        } 
        if(!tp->is_complete())
            error(m_cpp.peek(), "Parameter declaration with an incomplete type");
        // if name is nullptr means an abstract declarator
        params.push_back(m_curr->declare(name, tp, 0));
    } while(m_cpp.test(Comma));
    std::swap(s, m_curr);
    m_cpp.expect(RightParen);
    return make_func(ret, std::move(params), vaarg);
}

/*---------------------------------------------------.
|   enum_specifier                                   |
|       : ENUM '{' enumerator_list '}'               |
|       | ENUM IDENTIFIER '{' enumerator_list '}'    |
|       | ENUM IDENTIFIER                            |
|       ;                                            |
`---------------------------------------------------*/
type_enum* parser::enum_specifier() {
    // KeyEnum is recognized
    auto tok = m_cpp.get();
    type_enum *tp = nullptr;
    if(tok->m_attr == Identifier) {
        auto tag = m_curr->find_tag_current(tok);
        if(tag && tag->m_type->is_enum())
            tp = tag->m_type->to_enum();
        else {
            tp = make_enum();
            m_curr->declare_tag(tok, tp);
        }
        if(m_cpp.test(BlockOpen)) {
            enumerator_list(); // BlockClose handled here
            tp->set_complete(true);
        }
    } else {
        tp = make_enum();
        m_cpp.expect(BlockOpen);
        enumerator_list(); // BlockClose handled here
        tp->set_complete(true);
    }
    return tp;
}

/*------------------------------------------./+----------------------------------------------.
|   enumerator_list                         ||   enumerator                                  |
|       : enumerator                        ||       : IDENTIFIER                            |
|       | enumerator_list ',' enumerator    ||       | IDENTIFIER '=' constant_expression    |
|       ;                                   ||       ;                                       |
`------------------------------------------+/`----------------------------------------------*/
void parser::enumerator_list() {
    int curr = 0;
    for(;;) {
        if(m_cpp.test(BlockClose)) break;
        auto tok = m_cpp.get(Identifier);
        if(m_cpp.test(Assign))
            curr = eval_long(conditional_expr());
        m_curr->declare(tok, curr++);
        if(m_cpp.test(Comma)) continue;
    }
}

/*----------------------------------------./+--------------------------------------------.
|   initializer                           ||   initializer_list                          |
|       : assignment_expression           ||       : initializer                         |
|       | '{' initializer_list '}'        ||       | initializer_list ',' initializer    |
|       | '{' initializer_list ',' '}'    ||       ;                                     |
|       ;                                 |`--------------------------------------------+/
`----------------------------------------*/
init_list parser::initializer(qual_type tp) {
    init_list l{};
    auto tok = m_cpp.get();
    if(tok->is(BlockOpen)) {
        if(tp->is_array())
            array_initializer(l, tp->to_array());
        else if(tp->is_struct())
            struct_initializer(l, tp->to_struct());
        else 
            error(tok, "Expecting an aggregate type");
    } else if(tok->is(String) && tp->is_array()) {
        auto arr_type = tp->to_array();
        auto elem_type = arr_type->get();
        std::string str = tok->to_string();
        if(!elem_type->is_arith() && !elem_type->to_arith()->is_char())
            error(tok, "Cannot initialize type \"%s\" with string", tp.to_string().c_str());
        if(arr_type->is_complete()) {
            if(arr_type->size() <= str.size())
                error(tok, "String is too long");
        } else 
            arr_type->set_len(str.size() + 1); // one extra for '\0'
        l.push_back(make_string(tok));
    } else {
        m_cpp.unget(tok);
        l.push_back(make_init(tp, assignment_expr()));
    }
    return l;
}

void parser::array_initializer(init_list &l, type_array *arr) {
    unsigned int index = 0, length = arr->length();
    auto base = arr->to_derived()->get();
    while(!m_cpp.test(BlockClose)) {
        if(m_cpp.test(Dot))
            error(m_cpp.peek(), "Member designator in array initialization");
        if(m_cpp.test(LeftSubscript)) {
            auto offset = conditional_expr();
            auto temp = eval_long(offset);
            if(temp < index) 
                error(offset->m_tok, "Invalid offset expression");
            for(; index < temp; ++index)
                l.push_back(make_init(base, make_literal(0)));
            m_cpp.expect(RightSubscript);
            m_cpp.expect(Assign);
        } 
        l.splice(l.end(), initializer(base));
        ++index;
        if(!m_cpp.test(Comma)) {
            m_cpp.expect(BlockClose);
            break;
        }
    }
    if(!length)
        arr->set_len(index);
    else if(length < index)
        error(m_cpp.peek(), "Excess element number");
}

void parser::struct_initializer(init_list &l, type_struct *stru) {
    if(!stru->is_complete())
        error(m_cpp.peek(), "Initializer for incomplete struct");
    auto &&members = stru->get_members();
    auto it = members.begin();
    while(!m_cpp.test(BlockClose)) 
        l.splice(l.end(), initializer((*it)->m_type));
    for(; it != members.end(); ++it) 
        l.push_back(make_init((*it)->m_type, make_literal(0)));
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
stmt* parser::statement() {
    auto tok = m_cpp.get();
    switch(tok->m_attr) {
//        case KeyCase: case KeyDefault:
//            return label_stmt();
        case Semicolon: return make_stmt();
        case BlockOpen: return compound_stmt();
        case If: /*case KeySwitch: */
            return selection_stmt();
        case KeyFor: return for_loop();
        case KeyDo:  return do_while_loop();
        case KeyWhile: return while_loop();
        case KeyGoto: case KeyReturn: case KeyContinue: case KeyBreak:
            m_cpp.unget(tok);
            return jump_stmt();
        case Identifier: 
            if(m_cpp.peek(Colon)) {
                m_cpp.unget(tok);
                return label_stmt();
            }
        default: {
            m_cpp.unget(tok);
            auto res = make_expr_stmt(expr());
            m_cpp.expect(Semicolon);
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
stmt_compound* parser::label_stmt() {
    auto peek = m_cpp.peek();
    stmt_list l{};
    if(peek->m_attr == Identifier) {
        m_cpp.ignore();
        m_cpp.expect(Colon);
        auto dest = statement();
        auto name = peek->to_string();
        auto label = m_lmap.find(name);
        if(label != m_lmap.end())
            error(peek, "Redefinition of label \"%s\"", name);
        else 
            label = m_lmap.insert({name, make_label()}).first;
        l.push_back(label->second);
        l.push_back(dest);
    }
    return make_compound(m_curr, std::move(l));
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
stmt_compound* parser::compound_stmt(qual_type func) {
//    m_cpp.expect(BlockOpen); // extracted in caller function
    auto s = make_scope(m_curr);
    if(func) {
        // insert parameters
        for(auto &&param: func->to_func()->params())
            s->insert(param);
    }
    std::swap(s, m_curr);
    stmt_list l{};
    for(;;) {
        auto peek = m_cpp.peek();
        if(peek->is(BlockClose)) {
            m_cpp.ignore();
            break;
        } else if(decl_peek(peek, m_curr)) 
            decl(l);
        else
            l.push_back(statement());
    }
    std::swap(s, m_curr);
    return make_compound(s, std::move(l));
}

/*----------------------------------------------------------.
|   selection_statement                                     |
|       : IF '(' expression ')' statement                   |
|       | IF '(' expression ')' statement ELSE statement    |
|       | SWITCH '(' expression ')' statement               | // TODO
|       ;                                                   |
`----------------------------------------------------------*/
stmt_if* parser::selection_stmt() {
    //m_cpp.expect(If); // extracted in caller function
    m_cpp.expect(LeftParen);
    ast_expr *cond = expr();
    m_cpp.expect(RightParen);
    stmt *yes = statement();
    stmt *no = m_cpp.test(Else) ? statement() : nullptr;
    return make_if(cond, yes, no);
}

/*--------------------------------------------------------------------------------------.
|   iteration_statement                                                                 |
|       : WHILE '(' expression ')' statement                                            |
|       | DO statement WHILE '(' expression ')' ';'                                     |
|       | FOR '(' expression_statement expression_statement ')' statement               |
|       | FOR '(' expression_statement expression_statement expression ')' statement    |
|       ;                                                                               |
`--------------------------------------------------------------------------------------*/
#define ENTER_LOOP \
    auto backup_break = m_break;\
    auto backup_continue = m_continue;\
    m_break = make_label(); \
    m_continue = make_label(); \
    auto s = make_scope(m_curr); \
    std::swap(s, m_curr)

#define EXIT_LOOP \
    m_break = backup_break;\
    m_continue = backup_continue; \
    std::swap(s, m_curr)

stmt_compound* parser::while_loop() {
    //m_cpp.expect(KeyWhile); extracted in caller function
    m_cpp.expect(LeftParen);
    
    stmt_list l{};
    
    ENTER_LOOP;
    auto cond = expr();
    m_cpp.expect(RightParen);
    
    auto body = statement();
    auto body_label = make_label();
    auto _if = make_if(cond, make_jump(body_label), make_jump(m_break));
    auto loop = make_jump(m_continue);
    
    l.push_back(m_continue);
    l.push_back(_if);
    l.push_back(body_label);
    l.push_back(body);
    l.push_back(loop);
    l.push_back(m_break);
    EXIT_LOOP;
    
    return make_compound(s, std::move(l));
}

stmt_compound* parser::do_while_loop() {
    // m_cpp.expect(KeyDo); extracted in caller function
    stmt_list l{};
    
    ENTER_LOOP;
    auto body = statement();
    m_cpp.expect(KeyWhile);
    m_cpp.expect(LeftParen);
    auto cond = expr();
    m_cpp.expect(RightParen);
    m_cpp.expect(Semicolon);
    
    auto _if = make_if(cond, make_jump(m_continue), make_jump(m_break));
    
    l.push_back(m_continue);
    l.push_back(body);
    l.push_back(_if);
    l.push_back(m_break);
    EXIT_LOOP;
    
    return make_compound(s, std::move(l));
}

stmt_compound* parser::for_loop() {
    // m_cpp.expect(KeyFor); extracted in caller function
    m_cpp.expect(LeftParen);
    stmt_list l{};
    
    ENTER_LOOP;
    // after ENTER_LOOP, `s` is the former `m_curr`
    if(decl_peek(m_cpp.peek(), s))
        decl(l);
    else if(!m_cpp.test(Semicolon)) 
        l.push_back(make_expr_stmt(expr()));
    
    ast_expr *cond = nullptr;
    if(!m_cpp.test(Semicolon)) {
        cond = expr();
        m_cpp.expect(Semicolon);
    } else
        cond = make_literal(1);
    
    stmt *step = nullptr;
    if(!m_cpp.test(RightParen)) {
        step = make_expr_stmt(expr());
        m_cpp.expect(RightParen);
    } else 
        step = make_stmt();
    
    auto body = statement();
    auto body_label = make_label();
    auto if_label = make_label();
    auto _if = make_if(cond, make_jump(body_label), make_jump(m_break));
    auto loop = make_jump(if_label);
    
    l.push_back(if_label);
    l.push_back(_if);
    l.push_back(body_label);
    l.push_back(body);
    l.push_back(m_continue);
    l.push_back(step);
    l.push_back(loop);
    l.push_back(m_break);
    EXIT_LOOP;
    
    return make_compound(s, std::move(l));
}

#undef ENTER_LOOP
#undef EXIT_LOOP

/*---------------------------------.
|   jump_statement                 |
|       : GOTO IDENTIFIER ';'      |
|       | CONTINUE ';'             |
|       | BREAK ';'                |
|       | RETURN ';'               |
|       | RETURN expression ';'    |
|       ;                          |
`---------------------------------*/
stmt* parser::jump_stmt() {
    auto tok = m_cpp.get();
    stmt *res = nullptr;
    switch(tok->m_attr) {
        case KeyGoto: {
            tok = m_cpp.get(Identifier);
            auto name = tok->to_string();
            auto label = m_lmap.find(name);
            auto jump = make_jump(nullptr);
            if(label == m_lmap.end()) 
                m_unresolved.push_back({tok, jump});
            else
                jump->label = label->second;
            res = jump;
            break;
        }
        case KeyContinue:
            if(!m_continue)
                error(tok, "Use \"continue\" out of loop");
            res = make_jump(m_continue);
            break;
        case KeyBreak:
            if(!m_break)
                error(tok, "Use \"break\" out of loop");
            res = make_jump(m_break);
            break;
        case KeyReturn:
            if(!m_func)
                error(tok, "Use \"return\" out of function");
            if(m_cpp.peek()->m_attr == Semicolon)
                res = make_return(m_func);
            else 
                res = make_return(m_func, expr());
        default: break;
    }
    m_cpp.expect(Semicolon);
    return res;
}

/*-------------------------------------------------./+-------------------------------.
|   translation_unit                               ||   external_declaration         |
|       : external_declaration                     ||       : function_definition    |
|       | translation_unit external_declaration    ||       | declaration            |
|       ;                                          ||       ;                        |
`-------------------------------------------------+/`-------------------------------*/
void parser::translation_unit() {
    while(!m_cpp.test(Eof)) {
        if(m_cpp.test(Semicolon))
            continue;
        
        uint8_t stor = 0;
        auto base = decl_specifiers(stor);
        
        if(m_cpp.test(Semicolon)) {
            if((base->is_struct() || base->is_union() || base->is_enum()) && !stor)
                continue;
            error(m_cpp.peek(), "Expecting an identifier name");
        }
        
        auto decl_type = base;
        auto name = try_declarator(decl_type);
        
        if(!name)
            error(m_cpp.peek(), "Unexpected abstract declarator");
        
        if(decl_type->is_func()) {
            if(m_cpp.test(BlockOpen)) 
                m_tu.push_back(function_definition(name, decl_type, stor));
            else {
                m_tu.push_back(m_curr->declare_func(name, decl_type, stor, nullptr)->decl);
                m_cpp.expect(Semicolon);
            }
        } else {
            init_list inits{};
            if(m_cpp.test(Assign)) 
                inits = initializer(decl_type);
            auto var_decl = m_curr->declare(name, decl_type, stor)->decl;
            var_decl->inits = std::move(inits);
            if(m_cpp.test(Comma))
                init_declarators(m_tu, stor, base);
            m_cpp.expect(Semicolon);
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
stmt_decl* parser::function_definition(token *name, qual_type tp, uint8_t stor) {
    auto id = m_curr->find(name);
    if(id) {
        auto ptype = id->m_type; // previous declared type
        if(!ptype->is_func())
            error(name, "\"%s\" is not declared as function before", name->to_string());
        auto pfunc = reinterpret_cast<ast_func*>(id);
        if(pfunc->body)
            error(name, "\"%s\" has a definition", name->to_string());
        if(!ptype->compatible(tp))
            error(name, "Mismatched function signature");
        m_func = pfunc;
        // prototype may have anonymous parameter, update it
        id->m_type = tp;
        pfunc->body = compound_stmt(tp);
        for(auto &&resolve: m_unresolved) {
            auto lname = resolve.first->to_string();
            auto it = m_lmap.find(lname);
            if(it == m_lmap.end())
                error(resolve.first, "Unresolved label \"%s\"", lname);
            resolve.second->label = it->second;
        }
        m_lmap.clear();
        m_unresolved.clear();
        m_func = nullptr;
        return pfunc->decl;
    } else {
        auto func = m_curr->declare_func(name, tp, stor);
        m_func = func;
        func->body = compound_stmt(tp);
        for(auto &&resolve: m_unresolved) {
            auto lname = resolve.first->to_string();
            auto it = m_lmap.find(lname);
            if(it == m_lmap.end())
                error(resolve.first, "Unresolved label \"%s\"", lname);
            resolve.second->label = it->second;
        }
        m_lmap.clear();
        m_unresolved.clear();
        m_func = nullptr;
        return func->decl;
    }
}


unsigned int precedence(token *tok) {
    switch(tok->m_attr) {
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
        default: return min_prec;
    }
}

bool specifier_peek(token *tok, scope *s) {
    switch(tok->m_attr) {
        case KeyConst: case KeyVolatile:
        case KeyVoid: case KeyChar: case KeyShort:
        case KeyInt: case KeyLong: case KeyFloat:
        case KeyDouble: case KeySigned: case KeyUnsigned:
        case KeyStruct: case KeyUnion: case KeyEnum:
            return true;
        case Identifier: {
            auto id = s->find(tok);
            return id && id->to_type();
        }
        default: return false;
    }
}

bool decl_peek(token *tok, scope *s) {
    switch(tok->m_attr) {
        case KeyStatic:
        case KeyTypedef:
        case KeyRegister:
        case KeyInline:
        case KeyExtern:
            return true;
        default: return specifier_peek(tok, s);
    }
}
