#ifndef EVALUATOR_HPP
#define EVALUATOR_HPP

namespace Compiler {

namespace Semantic {
class Expr;
}

namespace ConstExpr {

long evalLong(Semantic::Expr*);

} // namespace ConstExpr
} // namespace Compiler

#endif // EVALUATOR_HPP
