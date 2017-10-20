#ifndef STMT_HPP
#define STMT_HPP

#include "utils/vector.hpp"

namespace Compiler {

namespace Semantic {

class Expr;

class Stmt {};

class DeclStmt : public Stmt {};

class CondStmt : public Stmt {};

class CompoundStmt : public Stmt {};

class LabelStmt : public Stmt {};

class JumpStmt : public Stmt {};

Stmt* makeStmt();

CondStmt* makeCondStmt(Expr *cond, Stmt *trueBranch, Stmt *falseBranch);

CompoundStmt* makeCompoundStmt(Utils::StmtList&&);

LabelStmt* makeLabelStmt();

JumpStmt* makeJumpStmt(LabelStmt *dest);

} // namespace Semantic
} // namespace Compiler

#endif // STMT_HPP
