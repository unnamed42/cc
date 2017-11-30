#ifndef __COMPILER_AST__
#define __COMPILER_AST__

#include "type.hpp"
#include "token.hpp"
#include "visitor.hpp"

#include <list>
#include <cstdint>
#include <iostream>

namespace compiler {

// base class of all, just an empty class
struct ast_node; 

// base class of all expressions, inherits from `ast_node`
// a `ast_expr` instance is a primary expression
// holds a `token` for error messaging, a `qual_type` indicating its type
struct ast_expr; 

// abstraction of all literals, inherits from `ast_expr`
// have a union to store its real value other than string from source file
struct ast_constant;
// abstraction of all identifiers, inherits from `ast_expr`
// have a pointer to its declaration
// a pure `ast_id` instance is a tag or a typedef name
struct ast_ident;
// abstraction of all objects, inherits from `ast_ident`
// an object **must** has its memory resource
struct ast_object;
// abstraction of enumerator, inherits from `ast_ident`
struct ast_enum;
// abstraction of all functions, inherits from `ast_ident`
struct ast_func;

// abstraction of all unary operations, inherits from `ast_expr`
// holds a `uint8_t` as opcode, a `ast_expr` as operand
struct ast_unary;
// abstraction of all cast operations, inherits from `ast_expr`
// the `qual_type` inherited from `ast_expr` is the type to be casted to
struct ast_cast;
// abstraction of all binary operations, inherits from `ast_expr`
// holds a `uint8_t` as opcode, two `ast_expr`s as lhs and rhs
struct ast_binary;
// abstraction of ?: operation, and if-else, for, while, do-while, inherits from `ast_expr`
// has 3 `ast_expr` as condition, true branch and false branch respectively
struct ast_ternary;
// abstraction of a function call, inherits from `ast_expr`
// the inherited `qual_type` indicates its return type
struct ast_call;

// abstraction of statements, base class of all stmt_* and ast_decl
struct stmt;
// compound statements, inherits from `stmt`
// has a list of ast_expr
struct stmt_compound;
// jump statements (goto, break, continue, return), inherits from `stmt`
struct stmt_jump;
// conditional statements (if-else, switch), inherits from `stmt`
struct stmt_if;
// expression statememts
struct stmt_expr;
// label statements
struct stmt_label;
// abstraction of all declarations, needs to allocate memory space for variables
// a declaration may be a struct/union/enum tag, a variable, a function
// have a initializer list to initialize the allocated memory space
struct stmt_decl;

// final state of parsing
//struct translation_unit;



typedef std::list<ast_expr*> arg_list;
typedef std::list<stmt*>     stmt_list;
typedef std::list<ast_expr*> init_list;

// all other opcodes are inherited from token_attr
enum opcode: uint32_t {
    NOP = 0, // Not an operation
    PostInc = 0xff000000, // avoid operator enumerator conflict
    PostDec,
    Mul,
    BitAnd,
    Member,
    Subscript, // binary: lhs - base, rhs - offset
    AddressOf, // unary operator '&'
    ArithmeticOf, // unary operator '+'
    Negate, // unary operator '-'
    Dereference, // unary operator '*'
    Cast, // binary
};

struct ast_node {
    virtual ~ast_node() = default;
    
    virtual void accept(visitor*) {}
};

struct ast_expr: public ast_node {
    token    *m_tok;
    qual_type m_type;
    
    ast_expr(token *t, qual_type tp)
        : m_tok(t), m_type(tp) {}
    virtual ~ast_expr() = default;
    
    virtual bool lvalue() const {return false;}
    virtual bool rvalue() const {return false;}
    
    virtual long valueof() const {
        error(m_tok, "Not a compile time constant");
        return 0;
    }
    
    void accept(visitor*) override {}
};

struct ast_constant: public ast_expr {
    union {
        unsigned long long ival;
        float fval;
        double dval;
        long double ldval;
        const char *str;
    };
    
    ast_constant(token *t, qual_type tp)
        :ast_expr(t, tp) {}
    
    bool rvalue() const override {return true;}
    
    long valueof() const override {return ival;}
    
    void accept(visitor *v) override {v->visit_constant(this);}
};

struct ast_ident: public ast_expr {
    // token, type inherited
    
    ast_ident(token *tok, qual_type tp)
        :ast_expr(tok, tp) {}
    
    virtual qual_type   to_type() {return m_type;}
    virtual ast_object* to_obj() {return nullptr;}
    virtual ast_func*   to_func() {return nullptr;}
    virtual ast_enum*   to_enum() {return nullptr;}
};

struct ast_object: public ast_ident {
    stmt_decl *decl; // location and resource of its declaration
    uint8_t    stor; // storage class
    uint8_t    bit_begin; // bit-field begin
    uint8_t    bit_width; // bit-field width
    const uint32_t id; // anonymous id, parameter-list may declare an anonymous object
    
    ast_object(token *tok, qual_type tp, stmt_decl *d, uint8_t s = 0, uint32_t i = 0, uint8_t begin = 0, uint8_t width = 0)
        :ast_ident(tok, tp), decl(d), stor(s), bit_begin(begin), bit_width(width), id(i) {}
    
    qual_type to_type() override {
        return stor == Typedef ? m_type : qual_null;
    }
    
    ast_object* to_obj() override {return this;}
    
    bool is_anony() const {return !id;}
    
    uint32_t bit_mask() const {
        static constexpr uint32_t max = 0xffffffffU;
        auto end = bit_begin + bit_width;
        return ((max << (32 - end)) >> (32 - bit_width)) << bit_begin;
    }
    
    bool lvalue() const override {return true;}
    bool rvalue() const override {return false;}
    
    long valueof() const override ;
    
    void accept(visitor *v) override {v->visit_object(this);}
};

struct ast_enum: public ast_ident {
    ast_constant val;
    
    ast_enum(token *t, int v)
        :ast_ident(t, qual_arith(Int)), val(nullptr, qual_arith(Int)) {val.ival = v;}
    
    ast_enum* to_enum() override {return this;}
    
    void accept(visitor *v) override {v->visit_enum(this);}
    long valueof() const override {return val.ival;}
};

struct ast_func: public ast_object {
    stmt_compound *body;
    
    ast_func(token *tok, qual_type tp, stmt_decl *d, uint8_t s = 0, stmt_compound *b = nullptr)
        :ast_object(tok, tp, d, s), body(b) {}
    
    ast_object* to_obj() override {return nullptr;}
    ast_func*   to_func() override {return this;}
    
    bool has_def() const {return body;}
    
    void accept(visitor *v) override {v->visit_func(this);}
};

struct ast_unary: public ast_expr {
    uint32_t  op;
    ast_expr *operand;
    
    ast_unary(token *t, qual_type tp, uint32_t o, ast_expr *p)
        :ast_expr(t, tp), op(o), operand(p) {}
    
    bool lvalue() const override {return op == Dereference || op == Inc || op == Dec;}
    bool rvalue() const override {return !lvalue();}
    
    void accept(visitor *v) override {v->visit_unary(this);}
    
    long valueof() const override {
        switch(op) {
            case Inc: 
            case Dec: 
            case Dereference: 
            case AddressOf: 
            case PostInc:
            case PostDec:
                error(m_tok, "Not a compile time constant");
            case Negate: return -operand->valueof();
            case ArithmeticOf: return operand->valueof();
            case BitNot: return ~operand->valueof();
            case LogicalNot: return !operand->valueof();
            default: 
                error(m_tok, "Unrecognized operator");
                return 0;
        }
    }
};

// The type to be casted to is stored in base class
struct ast_cast: public ast_expr {
    ast_expr *operand;
    
    ast_cast(token *tok, qual_type tp, ast_expr *oper)
        :ast_expr(tok, tp), operand(oper) {}
    
    void accept(visitor *v) override {v->visit_cast(this);} // TODO
    
    long valueof() const override {
        return operand->valueof();
    }
};

struct ast_binary: public ast_expr {
    uint32_t  op;
    ast_expr *lhs;
    ast_expr *rhs;
    
    ast_binary(token *t, qual_type tp, uint32_t o, ast_expr *l, ast_expr *r)
        :ast_expr(t, tp), op(o), lhs(l), rhs(r) {}
    bool lvalue() const override {return op == Subscript || op == Member || op == MemberPtr || is_assignment(op);}
    bool rvalue() const override {return !lvalue();}
    
    void accept(visitor *v) override {v->visit_binary(this);}
    
    long valueof() const override {
        #define LVAL lhs->valueof()
        #define RVAL rhs->valueof()
        switch(op) {
            case Comma: return RVAL; 
            // without dereferencing
            case Add: return LVAL + RVAL; 
            case Sub: return LVAL - RVAL; 
            case Div: return LVAL / RVAL; 
            case Mod: return LVAL % RVAL; 
            case BitOr: return LVAL | RVAL; 
            case BitXor: return LVAL ^ RVAL; 
            case LeftShift: return LVAL << RVAL; 
            case RightShift: return LVAL >> RVAL; 
            case LessThan: return LVAL < RVAL; 
            case LessEqual: return LVAL <= RVAL; 
            case GreaterThan: return LVAL > RVAL; 
            case GreaterEqual: return LVAL >= RVAL; 
            case Equal: return LVAL == RVAL; 
            case NotEqual: return LVAL != RVAL; 
            case LogicalAnd: return LVAL && RVAL; 
            case LogicalOr: return LVAL || RVAL; 
            // with dereferencing
            case Assign:
            case AddAssign:
            case SubAssign:
            case MulAssign:
            case DivAssign:
            case ModAssign:
            case BitAndAssign:
            case BitOrAssign:
            case BitXorAssign:
            case LeftShiftAssign:
            case RightShiftAssign:
            case Subscript:
                error(m_tok, "Not a compile time constant");
            default:
                return 0;
        }
        #undef RVAL
        #undef LVAL
    }
};

// can be used for ?:, if-else, while, do-while
struct ast_ternary: public ast_expr {
    ast_expr *cond;
    ast_expr *yes;
    ast_expr *no;
    
    ast_ternary(qual_type t, ast_expr *a1, ast_expr *a2, ast_expr *a3)
        :ast_expr(nullptr, t), cond(a1), yes(a2), no(a3) {}
    bool lvalue() const override {return yes->lvalue() && no->lvalue();}
    bool rvalue() const override {return !lvalue();}
    
    void accept(visitor *v) override {v->visit_ternary(this);}
    
    long valueof() const override {
        auto c = cond->valueof();
        auto y = yes->valueof();
        auto n = no->valueof();
        return c ? y : n;
    }
};

struct ast_call: public ast_expr {
    ast_func *func; // function designator
    arg_list  args;
    
    ast_call(token *tok, ast_func *f, arg_list &&list);
    
    bool rvalue() const override {return true;}
    
    void accept(visitor *v) override {v->visit_call(this);}
};

struct stmt: public ast_node {
    void accept(visitor *v) override {v->visit_stmt(this);}
};

struct stmt_label: public stmt {
    unsigned id;
    stmt_label() {
        static unsigned _id = 1;
        id = _id++;
    }
    
    void accept(visitor *v) override {v->visit_label(this);}
};

struct stmt_compound: public stmt {
    scope    *m_scope;
    stmt_list m_stmt;
    
    stmt_compound(scope *s, stmt_list &&l)
        :m_scope(s), m_stmt(std::move(l)) {}
    
    void accept(visitor *v) override {v->visit_compound(this);}
};

struct stmt_if: public stmt {
    ast_expr *cond;
    stmt *yes;
    stmt *no;
    
    stmt_if(ast_expr *c, stmt *y, stmt *n)
        :cond(c), yes(y), no(n) {}
    
    void accept(visitor *v) override {v->visit_if(this);}
};

struct stmt_jump: public stmt {
    stmt_label *label;
    
    stmt_jump(stmt_label *l):label(l) {}
    
    void accept(visitor *v) override {v->visit_jump(this);}
};

struct stmt_return: public stmt {
    ast_expr *val;
    
    stmt_return(ast_expr *e=nullptr)
        :val(e) {}
    
    void accept(visitor *v) override {v->visit_return(this);}
};

struct stmt_expr: public stmt {
    ast_expr *expr;
    
    stmt_expr(ast_expr *e):expr(e) {}
    
    void accept(visitor *v) override {v->visit_expr(this);}
};

struct stmt_decl: public stmt {
    ast_object *obj;
    init_list   inits;
    
    stmt_decl(ast_object *o)
        :obj(o), inits() {}
    stmt_decl(ast_object *o, init_list &&l)
        :obj(o), inits(std::move(l)) {}
    
    void accept(visitor *v) override {v->visit_decl(this);}
};

//struct translation_unit: public ast_node {
//    decl_list decls;
    
//    translation_unit(decl_list &&l)
//        :decls(std::move(l)) {}
//};

ast_constant* make_bool(token*);
ast_constant* make_string(token*);
ast_constant* make_number(token*);
ast_constant* make_char(token*);
ast_constant* make_sizeof(token*, qual_type);
ast_constant* make_sizeof(token*, ast_expr*);
ast_constant* make_literal(unsigned long long);

ast_ident*    make_ident(token*, qual_type);
ast_object*   make_object(token *tok, qual_type tp, stmt_decl *d, uint8_t s = 0, uint32_t i = 0, uint8_t begin = 0, uint8_t width = 0);
ast_func*     make_func(token *tok, qual_type tp, stmt_decl *d, uint8_t s = 0, stmt_compound *b = nullptr);
ast_enum*     make_enum(token *tok, int val);
//ast_label*    make_label(stmt* = nullptr);
//ast_label*    make_label(token*, stmt* = nullptr);

ast_unary*    make_unary(token*, ast_expr*, uint32_t);
ast_cast*     make_cast(token*, qual_type, ast_expr*);
ast_expr*     try_cast(ast_expr*, qual_type);

ast_expr*     make_init(qual_type, ast_expr*);

ast_binary*   make_binary(token*, ast_expr*, ast_expr*, uint32_t);
// attribute of second token must be `Identifier`
ast_binary*   make_member_access(token*, ast_expr*, token*);
ast_binary*   make_assignment(token*, ast_expr*, ast_expr*, uint32_t);

ast_ternary*  make_ternary(ast_expr*, ast_expr*, ast_expr*);

ast_call*     make_call(token*, ast_func*, arg_list&&);

// make an empty statement
stmt*          make_stmt();

stmt_decl*     make_decl(ast_object*);

stmt_expr*     make_expr_stmt(ast_expr*);

stmt_if*       make_if(ast_expr*, stmt*, stmt*);

stmt_compound* make_compound(scope*, stmt_list&&);

stmt_jump*   make_jump(stmt_label*);
stmt_label*  make_label();

stmt_return* make_return(ast_func*, ast_expr* = nullptr);

} // namespace compiler

#endif // __COMPILER_AST__
