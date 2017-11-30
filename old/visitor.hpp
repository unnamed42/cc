#ifndef __COMPILER_VISITOR__
#define __COMPILER_VISITOR__

namespace compiler {

struct ast_expr; 
struct ast_constant;
struct ast_ident;
struct ast_object;
struct ast_enum;
struct ast_func;
struct ast_unary;
struct ast_cast;
struct ast_binary;
struct ast_ternary;
struct ast_call;

struct stmt;
struct stmt_compound;
struct stmt_jump;
struct stmt_if;
struct stmt_expr;
struct stmt_decl;
struct stmt_label;
struct stmt_return;

class visitor {
    public:
        virtual ~visitor() = default;
        
        virtual void visit_constant(ast_constant*) = 0;
        virtual void visit_object(ast_object*) = 0;
        virtual void visit_enum(ast_enum*) = 0;
        virtual void visit_func(ast_func*) = 0;
        virtual void visit_unary(ast_unary*) = 0;
        virtual void visit_cast(ast_cast*) = 0;
        virtual void visit_binary(ast_binary*) = 0;
        virtual void visit_ternary(ast_ternary*) = 0;
        virtual void visit_call(ast_call*) = 0;
        
        virtual void visit_stmt(stmt*) = 0;
        virtual void visit_compound(stmt_compound*) = 0;
        virtual void visit_jump(stmt_jump*) = 0;
        virtual void visit_label(stmt_label*) = 0;
        virtual void visit_return(stmt_return*) = 0;
        virtual void visit_if(stmt_if*) = 0;
        virtual void visit_expr(stmt_expr*) = 0;
        virtual void visit_decl(stmt_decl*) = 0;
};

void print_top();

} // namespace compiler

#endif // __COMPILER_VISITOR__
