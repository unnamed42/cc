#ifndef __COMPILER_CODE_GENERATOR__
#define __COMPILER_CODE_GENERATOR__

#include "visitor.hpp"

#include <map>
#include <deque>
#include <string>
#include <fstream>

namespace compiler {

class IR: public visitor {
    public:
        typedef std::map<std::string, unsigned> mem_map;
        typedef std::deque<std::string>         stack_t;
    private:
        mem_map mem;
        stack_t stack;
        
        std::fstream file;
    private:
        void visit_constant(ast_constant*) override;
        void visit_object(ast_object*) override;
        void visit_enum(ast_enum*) override;
        void visit_func(ast_func*) override;
        void visit_unary(ast_unary*) override;
        void visit_cast(ast_cast*) override;
        void visit_binary(ast_binary*) override;
        void visit_ternary(ast_ternary*) override;
        void visit_call(ast_call*) override;
        
        void visit_stmt(stmt*) override;
        void visit_compound(stmt_compound*) override;
        void visit_jump(stmt_jump*) override;
        void visit_label(stmt_label*) override;
        void visit_return(stmt_return*) override;
        void visit_if(stmt_if*) override;
        void visit_expr(stmt_expr*) override;
        void visit_decl(stmt_decl*) override;
    private:
        std::string pop();
    public:
        IR(const char *loc)
            :mem(), stack(), file(loc, std::ios::out|std::ios::trunc) {}
};

} // namespace compiler

#endif // __COMPILER_CODE_GENERATOR__
