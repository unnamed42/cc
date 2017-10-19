#ifndef STMT_HPP
#define STMT_HPP

namespace Compiler {
namespace Semantic {

class Expr;

class Stmt {};

class DeclStmt : public Stmt {};

class CondStmt : public Stmt {};

class CompoundStmt : public Stmt {};




Stmt* makeStmt();

CondStmt* makeCondStmt(Expr *cond, Stmt *trueBranch, Stmt *falseBranch);

} // namespace Semantic
} // namespace Compiler

#endif // STMT_HPP
