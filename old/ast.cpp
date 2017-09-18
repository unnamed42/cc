#include "ast.hpp"
#include "scope.hpp"
#include "error.hpp"
#include "token.hpp"
#include "mempool.hpp"

using namespace compiler;

static mempool<ast_unary>    unary_pool{};
static mempool<ast_binary>   binary_pool{};
static mempool<ast_ternary>  ternary_pool{};
static mempool<ast_constant> const_pool{};
static mempool<ast_cast>     cast_pool{};
static mempool<ast_call>     call_pool{};
static mempool<ast_ident>    ident_pool{};
static mempool<ast_object>   obj_pool{};
static mempool<ast_func>     func_pool{};
static mempool<ast_enum>     enum_pool{};

static mempool<stmt_compound> compound_pool{};
static mempool<stmt_if>       if_pool{};
static mempool<stmt_jump>     jump_pool{};
static mempool<stmt_return>   return_pool{};
static mempool<stmt_expr>     expr_pool{};
static mempool<stmt_decl>     decl_pool{};
static mempool<stmt_label>    label_pool{};

static qual_type usual_arith_conversion(qual_type&, qual_type&);

static uint32_t integer_suffix(const std::string&);
static uint32_t float_suffix(const std::string&);


ast_call::ast_call(token *tok, ast_func *f, arg_list &&l)
    :ast_expr(tok, f->m_type->to_func()->return_type()), func(f), args(std::move(l)) {}

ast_constant* compiler::make_bool(token *tok) {
    bool val = tok->m_attr == KeyTrue;
    auto res = new (const_pool.malloc()) ast_constant(tok, qual_arith(Bool));
    res->ival = val;
    return res;
}

ast_constant* compiler::make_char(token *tok) {
    return new (const_pool.malloc()) ast_constant(tok, qual_arith(Bool));
}

ast_constant* compiler::make_string(token *tok) {
    auto ptr = make_pointer(make_arith(Char), Const);
    auto qual = make_qual(ptr);
    auto res = new (const_pool.malloc()) ast_constant(tok, qual);
    res->str = tok->to_string();
    return res;
}

ast_constant* compiler::make_number(token *tok) {
    std::string str = tok->to_string(), suffix{};
    uint32_t tp = 0;
    int base = 10;
    mark_pos(tok);
    for(auto rit = str.rbegin(); rit != str.rend(); ++rit) {
        auto ch = *rit;
        if(std::isxdigit(ch) || ch == 'p' || ch == 'P' || ch == '.') 
            break;
        suffix += ch;
        str.pop_back();
    }
    if(str[0] == '0') {
        switch(str[1]) {
            case 'x': case 'X': base = 16; str = str.substr(2); break;
            case 'b': case 'B': base = 2; str = str.substr(2); break;
            case '0': case '1': case '2': case '3': 
            case '4': case '5': case '6': case '7':
                base = 8; str = str.substr(1); break;
            case '8': case '9': 
            case '\0': break; // in this case, the number is just 0
            default: error(tok, "Invalid character in pp-number");
        }
    }
    if(tok->is(PPNumber)) 
        tp = integer_suffix(suffix);
    else // tok->m_attr == PPFloat
        tp = float_suffix(suffix);
    auto res_type = qual_arith(tp);
    auto res = new (const_pool.malloc()) ast_constant(tok, res_type);
    try {
        switch(tp) {
            case Int: case Unsigned|Int: case Long: case Unsigned|Long:
            case LLong: case Unsigned|LLong:
                res->ival = stoull(str, nullptr, base); break;
            case Float: res->fval = stof(str); break;
            case Double: res->dval = stod(str); break;
            case Long|Double: res->ldval = stold(str); break;
        }
    } catch(std::invalid_argument &e) {
        const_pool.free(res);
        error(tok, "Malformed number");
        return nullptr;
    } catch(std::out_of_range &e) {
        const_pool.free(res);
        error(tok, "Number is larger than what type %s can hold", res_type.to_string().c_str());
        return nullptr;
    }
    return res;
} 

/* C99 6.5.3.4 The sizeof operator
 * 
 * The sizeof operator shall not be applied to an expression that has function type or an
 * incomplete type, to the parenthesized name of such a type, or to an expression that
 * designates a bit-field member.
 */
ast_constant* compiler::make_sizeof(token *tok, qual_type tp) {
    if(tp->is_func() || !tp->is_complete())
        error(tok, "Cannot take size of function or incomplete type");
    
    auto res = new (const_pool.malloc()) ast_constant(tok, qual_arith(Unsigned|Long));
    res->ival = tp->size();
    
    return res;
}

ast_constant* compiler::make_sizeof(token *tok, ast_expr *expr) {
    return make_sizeof(tok, expr->m_type);
}

ast_constant* compiler::make_literal(unsigned long long l) {
    auto res = new (const_pool.malloc()) ast_constant(nullptr, qual_arith(Int));
    res->ival = l;
    return res;
}

ast_ident* compiler::make_ident(token *tok, qual_type tp) {
    return new (ident_pool.malloc()) ast_ident(tok, tp);
}

ast_object* compiler::make_object(token *tok, qual_type tp, stmt_decl *d, uint8_t stor, uint32_t id, uint8_t bit_begin, uint8_t bit_width) {
    return new (obj_pool.malloc()) ast_object(tok, tp, d, stor, id, bit_begin, bit_width);
}

ast_func* compiler::make_func(token *tok, qual_type tp, stmt_decl *d, uint8_t s, stmt_compound *b) {
    return new (func_pool.malloc()) ast_func(tok, tp, d, s, b);
}

ast_enum* compiler::make_enum(token *tok, int val) {
    return new (enum_pool.malloc()) ast_enum(tok, val);
}

//ast_label* compiler::make_label(stmt *dest) {
//    return new (label_pool.malloc()) ast_label(dest);
//}

//ast_label* compiler::make_label(token *tok, stmt *dest) {
//    return new (label_pool.malloc()) ast_label(tok, dest);
//}

/* C99 6.5.3.1 Prefix increment and decrement operators
 * 
 * The operand of the prefix increment or decrement operator shall have qualified or
 * unqualified real or pointer type and shall be a modifiable lvalue.
 * 
 * C99 6.5.3.2 Address and indirection operators
 * 
 * The operand of the unary & operator shall be either a function designator, the result of a
 * [] or unary * operator, or an lvalue that designates an object that is not a bit-field and is
 * not declared with the register storage-class specifier.
 * 
 * The operand of the unary * operator shall have pointer type.
 * 
 * C99 6.5.3.3 Unary arithmetic operators
 * 
 * The operand of the unary + or - operator shall have arithmetic type; of the ~ operator,
 * integer type; of the ! operator, scalar type.
 * 
 * The result of the unary + operator is the value of its (promoted) operand. The integer
 * promotions are performed on the operand, and the result has the promoted type.
 * 
 * The result of the unary - operator is the negative of its (promoted) operand. The integer
 * promotions are performed on the operand, and the result has the promoted type.
 * 
 * The result of the ~ operator is the bitwise complement of its (promoted) operand (that is,
 * each bit in the result is set if and only if the corresponding bit in the converted operand is
 * not set). The integer promotions are performed on the operand, and the result has the
 * promoted type. If the promoted type is an unsigned type, the expression ~E is equivalent
 * to the maximum value representable in that type minus E.
 * 
 * The result of the logical negation operator ! is 0 if the value of its operand compares
 * unequal to 0, 1 if the value of its operand compares equal to 0. The result has type int.
 * The expression !E is equivalent to (0==E).
 */
ast_unary* compiler::make_unary(token *t, ast_expr *e, uint32_t op) {
    #define ERROR_MSG error(t, "Invalid operand")
    auto tp = e->m_type;
    switch(op) {
        case Negate: case ArithmeticOf: 
            if(!tp->is_arith()) ERROR_MSG;
            tp = make_qual(tp->to_arith()->promote());
            break;
        case BitNot:
            if(!tp->to_arith() && !tp->to_arith()->is_integer()) 
                ERROR_MSG;
            tp = make_qual(tp->to_arith()->promote());
            break;
        case LogicalNot:
            if(!tp->is_scalar()) ERROR_MSG;
            tp = make_qual(make_arith(Int));
            break;
        case Dereference:
            if(!tp.decay()->is_pointer())
                ERROR_MSG;
            if(!tp->is_func()) 
                tp = tp->to_derived()->get();
            break;
        case AddressOf:
            if(!e->lvalue() && !tp->is_func()) ERROR_MSG;
            tp = qual_pointer(tp);
            break;
        case Inc: case Dec:
            if(e->lvalue() && (tp->is_scalar())) ;
            else ERROR_MSG;
            break;
    }
    #undef ERROR_MSG
    return new (unary_pool.malloc()) ast_unary(t, tp, op, e);
}


/* C99 6.5.4 Cast operators
 * 
 * Unless the type name specifies a void type, the type name shall specify qualified or
 * unqualified scalar type and the operand shall have scalar type.
 * 
 * Conversions that involve pointers, other than where permitted by the constraints of
 * 6.5.16.1, shall be specified by means of an explicit cast.
 */
ast_cast* compiler::make_cast(token *tok, qual_type tp, ast_expr *expr) {
    if(tp->is_void())
        error(tok, "Cannot cast to void type");
    
    if(!tp->is_scalar())
        error(tok, "The type casted to should be scalar type");
    
    return new (cast_pool.malloc()) ast_cast(tok, tp, expr);
}

ast_expr* compiler::try_cast(ast_expr *expr, qual_type dest) {
//    auto src = expr->m_type; // source type
    
//    auto srcp = src->to_pointer();
//    auto destp = dest->to_pointer();
//    if(srcp && destp) {
//        if(srcp->is_voidptr() || destp->is_voidptr())
//            return expr;
//    }
//    if(!dest->compatible(src)) 
//        expr = make_cast(expr->m_tok, dest, expr);
//    return expr;
    // ensure modifiable
    
    auto &ltype = dest, rtype = expr->m_type;
    
    auto &&tok = expr->m_tok;
    if(ltype->is_arith()) {
        if(!rtype->is_arith() && !ltype->to_arith()->is_bool())
            error(tok, "The right hand operand is required to be an arithmetic type");
    } else if(ltype->is_pointer()) {
        rtype = rtype.decay();
        if(!rtype->is_pointer()) 
            error(tok, "Cannot convert type \"%s\" to a pointer type", rtype.to_string().c_str());
        // get type pointed to
        auto lptr = ltype->to_derived()->get();
        auto rptr = rtype->to_derived()->get();
        // get qualifiers of type pointed to
        auto lqual = lptr.qual(), rqual = rptr.qual();
        // the type pointed to by the left has all the qualifiers of the type pointed to by the right;
        if(~lqual & rqual)
            error(tok, "The cast loses qualifier");
        else if(!lptr->compatible(rptr) && !(lptr->is_void() || rptr->is_void()))
            error(tok, "Cannot convert \"%s\" to type \"%s\"", rtype.to_string().c_str(), ltype.to_string().c_str());
    } else if(!ltype->compatible(rtype))
        error(tok, "Cannot convert \"%s\" to type \"%s\"", rtype.to_string().c_str(), ltype.to_string().c_str());
    
    if(ltype->compatible(rtype))
        return expr;
    return make_cast(tok, dest, expr);
}

ast_expr* compiler::make_init(qual_type tp, ast_expr *e) {
    return try_cast(e, tp);
}

/* C99 6.5.2.1 Array subscripting
 * 
 * A postfix expression followed by an expression in square brackets [] is a subscripted
 * designation of an element of an array object. The definition of the subscript operator []
 * is that E1[E2] is identical to (*((E1)+(E2))). 
 * 
 * C99 6.5.5 Multiplicative operators
 * 
 * Each of the operands shall have arithmetic type. The operands of the % operator shall
 * have integer type.
 * 
 * C99 6.5.6 Additive operators
 * 
 * For addition, either both operands shall have arithmetic type, or one operand shall be a
 * pointer to an object type and the other shall have integer type. (Incrementing is
 * equivalent to adding 1.)
 * 
 * For subtraction, one of the following shall hold:
 * 
 * — both operands have arithmetic type;
 *   This is often called "truncation toward zero".
 * — both operands are pointers to qualified or unqualified versions of compatible object
 *   types; or
 * — the left operand is a pointer to an object type and the right operand has integer type.
 *   (Decrementing is equivalent to subtracting 1.)
 * 
 * C99 6.5.7 Bitwise shift operators
 * 
 * Each of the operands shall have integer type.
 * 
 * The integer promotions are performed on each of the operands. The type of the result is
 * that of the promoted left operand. If the value of the right operand is negative or is
 * greater than or equal to the width of the promoted left operand, the behavior is undefined.
 * 
 * C99 6.5.8 Relational operators
 * 
 * One of the following shall hold:
 * 
 * — both operands have real type;
 * — both operands are pointers to qualified or unqualified versions of compatible object
 *   types; or
 * — both operands are pointers to qualified or unqualified versions of compatible
 *   incomplete types.
 * 
 * If both of the operands have arithmetic type, the usual arithmetic conversions are
 * performed.
 * 
 * C99 6.5.9 Equality operators
 * 
 * One of the following shall hold:
 * 
 * — both operands have arithmetic type;
 * — both operands are pointers to qualified or unqualified versions of compatible types;
 * — one operand is a pointer to an object or incomplete type and the other is a pointer to a
 *   qualified or unqualified version of void; or
 * — one operand is a pointer and the other is a null pointer constant.
 * 
 * C99 6.5.10 Bitwise AND operator
 * C99 6.5.11 Bitwise exclusive OR operator
 * C99 6.5.12 Bitwise inclusive OR operator
 * 
 * Each of the operands shall have integer type.
 * 
 * C99 6.5.13 Logical AND operator
 * C99 6.5.14 Logical OR operator
 * 
 * Each of the operands shall have scalar type.
 */
ast_binary* compiler::make_binary(token *tok, ast_expr *lhs, ast_expr *rhs, uint32_t op) {
    auto ltype = lhs->m_type.decay();
    auto rtype = rhs->m_type.decay();
    
    auto tp = ltype;
    switch(op) {
        case Subscript: tp = lhs->m_type->to_derived()->get(); break;
    }
    
    if(ltype->is_pointer()) 
        rhs = make_binary(nullptr, make_literal(rhs->m_type->size()), rhs, Mul);
    
    return new (binary_pool.malloc()) ast_binary(tok, tp, op, lhs, rhs);
}

ast_binary* compiler::make_member_access(token *tok, ast_expr *base, token *member) {
    // opcode `Member` 's token attribute `Dot`
    uint32_t op = tok->is(MemberPtr) ? MemberPtr : Member;
    
    auto stype = base->m_type;
    if(op == MemberPtr) {
        if(!stype->is_pointer())
            error(tok, "A pointer type required");
        stype = stype->to_derived()->get();
    } else if(op == Member) {
        if(!stype->is_struct() && !stype->is_union())
            error(tok, "A struct/union type required");
    }
    if(!stype->is_struct() && !stype->is_union())
        error(tok, "A structure/union type required");
    
    auto sc = stype->to_struct()->get_scope();
    auto id = sc->find_current(member);
    if(!id) 
        error(member, "\"%s\" is not a member of struct/union type \"%s\"", member->to_string(), stype.to_string().c_str());
    
    /* C99 6.5.2.3 Structure and union members
     * 
     * If the first expression is a pointer to a qualified type, the result has 
     * the so-qualified version of the type of the designated member.
     */
    auto ret_type = id->m_type;
    ret_type.add_qual(stype.qual());
    
    return new (binary_pool.malloc()) ast_binary(tok, ret_type, op, base, id);
}

/* C99 6.5.16 Assignment operators
 * 
 * An assignment operator shall have a modifiable lvalue as its left operand.
 * 
 * C99 6.5.16.1 Simple assignment
 * 
 * One of the following shall hold:
 * — the left operand has qualified or unqualified arithmetic type and the right has
 *   arithmetic type;
 * — the left operand has a qualified or unqualified version of a structure or union type
 *   compatible with the type of the right;
 * — both operands are pointers to qualified or unqualified versions of compatible types,
 *   and the type pointed to by the left has all the qualifiers of the type pointed to by the
 *   right;
 * — one operand is a pointer to an object or incomplete type and the other is a pointer to a
 *   qualified or unqualified version of void, and the type pointed to by the left has all
 *   the qualifiers of the type pointed to by the right;
 * — the left operand is a pointer and the right is a null pointer constant; or
 * — the left operand has type _Bool and the right is a pointer.
 * 
 * C99 6.5.16.2 Compound assignment
 * 
 * For the operators += and -= only, either the left operand shall be a pointer to an object
 * type and the right shall have integer type, or the left operand shall have qualified or
 * unqualified arithmetic type and the right shall have arithmetic type.
 * 
 * For the other operators, each operand shall have arithmetic type consistent with those
 * allowed by the corresponding binary operator.
 */
ast_binary* compiler::make_assignment(token *tok, ast_expr *lhs, ast_expr *rhs, uint32_t op) {
    
    // ensure modifiable
    if(!lhs->lvalue()) 
        error(tok, "Left hand operand of assignment must be an lvalue expression");
    else if(lhs->m_type.is_const())
        error(tok, "Cannot assign to a const qualified expression");
    
    auto ltype = lhs->m_type, rtype = rhs->m_type;
    
    if(ltype->is_arith()) {
        if(!rtype->is_arith() && !ltype->to_arith()->is_bool())
            error(tok, "The right hand operand is required to be an arithmetic type");
    } else if(ltype->is_pointer()) {
        rtype = rtype.decay();
        if(!rtype->is_pointer()) 
            error(tok, "Cannot convert type \"%s\" to a pointer type", rtype.to_string().c_str());
        // get type pointed to
        auto lptr = ltype->to_derived()->get();
        auto rptr = rtype->to_derived()->get();
        // get qualifiers of type pointed to
        auto lqual = lptr.qual(), rqual = rptr.qual();
        // the type pointed to by the left has all the qualifiers of the type pointed to by the right;
        if(~lqual & rqual)
            error(tok, "The assignment loses qualifier");
        else if(!lptr->compatible(rptr) && !(lptr->is_void() || rptr->is_void()))
            error(tok, "Cannot assign \"%s\" to type \"%s\"", rtype.to_string().c_str(), ltype.to_string().c_str());
    } else if(!ltype->compatible(rtype))
        error(tok, "Cannot assign \"%s\" to type \"%s\"", rtype.to_string().c_str(), ltype.to_string().c_str());
    
    if(op != Assign) {
        op >>= (sizeof(char) << 3); // get the operation
        if(op == Star) op = Mul;
        rhs = make_binary(tok, lhs, rhs, op);
    }
    
    return new (binary_pool.malloc()) ast_binary(tok, lhs->m_type, Assign, lhs, rhs);
}

/* C99 6.5.15 Conditional operator
 * 
 * The first operand shall have scalar type.
 * 
 * One of the following shall hold for the second and third operands:
 * — both operands have arithmetic type;
 * — both operands have the same structure or union type;
 * — both operands have void type;
 * — both operands are pointers to qualified or unqualified versions of compatible types;
 * — one operand is a pointer and the other is a null pointer constant; or
 * — one operand is a pointer to an object or incomplete type and the other is a pointer to a
 *   qualified or unqualified version of void.
 */
ast_ternary* compiler::make_ternary(ast_expr *cond, ast_expr *yes, ast_expr *no) {
    auto tp = cond->m_type;
    
    if(!tp->is_scalar())
        error(cond->m_tok, "Requiring a scalar type expression");
    
    auto yes_t = yes->m_type, no_t = no->m_type;
    if(!no_t->compatible(yes_t))
        no = try_cast(no, yes_t);
    
    return new (ternary_pool.malloc()) ast_ternary(yes->m_type, cond, yes, no);
}



/* C99 6.5.2.2 Function calls
 * 
 * The expression that denotes the called function shall have type pointer to function
 * returning void or returning an object type other than an array type.
 * 
 * If the expression that denotes the called function has a type that includes a prototype, the
 * number of arguments shall agree with the number of parameters. Each argument shall
 * have a type such that its value may be assigned to an object with the unqualified version
 * of the type of its corresponding parameter.
 */
ast_call* compiler::make_call(token *tok, ast_func *func, arg_list &&args) {
    auto tp = func->m_type;
    
    if(tp->is_pointer()) 
        tp = tp->to_pointer()->get();
    if(!tp->is_func())
        error(tok, "Invalid function call");
    
    auto &&params = tp->to_func()->params();
    auto pit = params.begin();
    auto ait = args.begin();
    auto va = tp->to_func()->is_vaarg();
    
    qual_type param_t, arg_t;
    for(; pit != params.end(); ++pit, ++ait) {
        if(ait == args.end()) 
            error(tok, "Too few arguments");
        param_t = (*pit)->m_type;
        arg_t   = (*ait)->m_type;
        
        *ait = try_cast(*ait, param_t);
    }
    if(pit == params.end() && ait != args.end() && !va) {
        auto atok = (*ait)->m_tok;
        error(atok, "Too many arguments");
    }
    
    return new (call_pool.malloc()) ast_call(tok, func, std::move(args));
}

stmt* compiler::make_stmt() {
    static stmt empty;
    return &empty;
}

stmt_decl* compiler::make_decl(ast_object *o) {
    return new (decl_pool.malloc()) stmt_decl(o);
}

stmt_expr* compiler::make_expr_stmt(ast_expr *expr) {
    return new (expr_pool.malloc()) stmt_expr(expr);
}

stmt_if* compiler::make_if(ast_expr *cond, stmt *yes, stmt *no) {
    if(!cond->m_type->is_scalar())
        error(cond->m_tok, "Expecting a scalar type expression");
    
    return new (if_pool.malloc()) stmt_if(cond, yes, no);
}

stmt_compound* compiler::make_compound(scope *s, stmt_list &&l) {
    return new (compound_pool.malloc()) stmt_compound(s, std::move(l));
}

stmt_jump* compiler::make_jump(stmt_label *dest) {
    return new (jump_pool.malloc()) stmt_jump(dest);
}

stmt_label* compiler::make_label() {
    return new (label_pool.malloc()) stmt_label();
}

stmt_return* compiler::make_return(ast_func *func, ast_expr *ret) {
    auto ret_type = func->m_type->to_func()->return_type();
    auto space = return_pool.malloc();
    
    if(ret)
        return new (space) stmt_return(try_cast(ret, ret_type));
    return new (space) stmt_return(ret);
}

/* C99 6.3.1.8 Usual arithmetic conversions
 * 
 * First, if the corresponding real type of either operand is long double, the other
 * operand is converted, without change of type domain, to a type whose
 * corresponding real type is long double.
 * 
 * Otherwise, if the corresponding real type of either operand is double, the other
 * operand is converted, without change of type domain, to a type whose
 * corresponding real type is double.
 * 
 * Otherwise, if the corresponding real type of either operand is float, the other
 * operand is converted, without change of type domain, to a type whose
 * corresponding real type is float.
 * 
 * Otherwise, the integer promotions are performed on both operands. Then the
 * following rules are applied to the promoted operands:
 * 
 *     If both operands have the same type, then no further conversion is needed.
 * 
 *     Otherwise, if both operands have signed integer types or both have unsigned
 *     integer types, the operand with the type of lesser integer conversion rank is
 *     converted to the type of the operand with greater rank.
 * 
 *     Otherwise, if the operand that has unsigned integer type has rank greater or
 *     equal to the rank of the type of the other operand, then the operand with
 *     signed integer type is converted to the type of the operand with unsigned
 *     integer type.
 * 
 *     Otherwise, if the type of the operand with signed integer type can represent
 *     all of the values of the type of the operand with unsigned integer type, then
 *     the operand with unsigned integer type is converted to the type of the
 *     operand with signed integer type.
 * 
 *     Otherwise, both operands are converted to the unsigned integer type
 *     corresponding to the type of the operand with signed integer type.
 */
qual_type usual_arith_conversion(qual_type &lhs, qual_type &rhs) {
    
    return lhs;
}

uint32_t integer_suffix(const std::string &str) {
    uint32_t tp = 0;
    for(auto &&c: str) {
        switch(c) {
            case 'u': case 'U':
                tp = apply_spec(tp, Unsigned);
                break;
            case 'l': case 'L':
                tp = apply_spec(tp, Long);
                break;
            default: 
                error(*epos, "Unknown integer suffix");
        }
    }
    
    if(!tp) tp = Int;
    return tp;
}

/* C99 6.4.4.2 Floating constants
 * 
 * An unsuffixed floating constant has type double. If suffixed by the letter f or F, it has
 * type float. If suffixed by the letter l or L, it has type long double.
 */
uint32_t float_suffix(const std::string &str) {
    uint32_t tp = 0;
    for(auto &&c: str) {
        switch(c) {
            case 'f': case 'F':
                tp = apply_spec(tp, Float);
                break;
            case 'l': case 'L':
                tp = apply_spec(tp, Double);
                tp = apply_spec(tp, Long);
                break;
            default: 
                error(*epos, "Unknown floating-point suffix");
        }
    }
    if(!tp) tp = Double;
    return tp;
}

long ast_object::valueof() const {
    auto inits = decl->inits;
    if(inits.empty()) 
        error(m_tok, "Not a compile time constant");
    return inits.front()->valueof();
}
