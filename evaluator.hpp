#ifndef __COMPILER_EVALUATOR__
#define __COMPILER_EVALUATOR__

namespace compiler {

struct ast_expr;

long eval_long(ast_expr*);

} // namespace compiler

#endif // __COMPILER_EVALUATOR__
