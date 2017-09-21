#include "utils/mempool.hpp"
#include "lexical/token.hpp"
#include "semantic/expr.hpp"

namespace impl = Compiler::Semantic;

using namespace Compiler::Utils;
using namespace Compiler::Lexical;
using namespace Compiler::Semantic;
using namespace Compiler::Diagnostic;

Expr::Expr(Token *tok) noexcept : tok(tok) {}
Token* Expr::token() noexcept { return tok; }
SourceLoc* Expr::sourceLoc() noexcept { return tok->sourceLoc(); }

UnaryExpr::UnaryExpr(Token *tok, OpCode op, Expr *expr) noexcept 
    : Expr(tok), op(op), expr(expr) {}

BinaryExpr::BinaryExpr(Token *tok, OpCode op, Expr *lhs, Expr *rhs) noexcept 
    : Expr(tok), op(op), lhs(lhs), rhs(rhs) {}

TernaryExpr::TernaryExpr(Token *tok, Expr *cond, Expr *yes, Expr *no) noexcept
    : Expr(tok), cond(cond), yes(yes), no(no) {}

UnaryExpr* impl::makeUnary(Token *tok, OpCode op, Expr *expr) {
    return new (pool.allocate(sizeof(UnaryExpr))) UnaryExpr(tok, op, expr);
}

BinaryExpr* impl::makeBinary(Token *tok, OpCode op, Expr *lhs, Expr *rhs) {
    return new (pool.allocate(sizeof(BinaryExpr))) BinaryExpr(tok, op, lhs, rhs);
}
