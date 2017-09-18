#include "ast.hpp"
#include "error.hpp"
#include "evaluator.hpp"

using namespace compiler;

long compiler::eval_long(ast_expr *e) {
    return e->valueof();
}
