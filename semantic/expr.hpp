#ifndef EXPR_HPP
#define EXPR_HPP

#include "utils/vector.hpp"
#include "semantic/qualtype.hpp"

#include <cstdint>

namespace Compiler {

namespace Utils {
class PtrList;
}
namespace Diagnostic {
struct SourceLoc;
}
namespace Lexical {
class Token;
}

namespace Semantic {

enum OpCode : uint32_t;

class FuncDecl;

class Expr {
    private:
        Lexical::Token *tok;
    public:
        Expr(Lexical::Token *) noexcept;
        virtual ~Expr() = default;
        
        Lexical::Token* token() noexcept;
        
        Diagnostic::SourceLoc* sourceLoc() noexcept;
};

class UnaryExpr : public Expr {
    private:
        OpCode op;
        Expr  *expr;
    public:
        UnaryExpr(Lexical::Token *tok, OpCode op, Expr *expr) noexcept;
};

class BinaryExpr : public Expr {
    private:
        OpCode op;
        Expr *lhs;
        Expr *rhs;
    public:
        BinaryExpr(Lexical::Token *tok, OpCode op, Expr *lhs, Expr *rhs) noexcept;
};

class TernaryExpr : public Expr {
    private:
        Expr *cond;
        Expr *yes;
        Expr *no;
    public:
        TernaryExpr(Lexical::Token *tok, Expr *cond, Expr *yes, Expr *no) noexcept;
};

class CastExpr : public Expr {
    private:
        Expr    *from;
        QualType to;
};

class CallExpr : public Expr {
    private:
        
};

class ObjectExpr : public Expr {};

class ConstantExpr : public Expr {};

UnaryExpr*   makeUnary(Lexical::Token *tok, OpCode op, Expr *expr);
UnaryExpr*   makeSizeOf(Lexical::Token *opSizeOf, Expr *expr);
UnaryExpr*   makeSizeOf(Lexical::Token *opSizeOf, Type *type);

CastExpr*    makeCast(Expr *from, QualType to);

BinaryExpr*  makeBinary(Lexical::Token *tok, OpCode op, Expr *lhs, Expr *rhs);
BinaryExpr*  makeMemberAccess(Lexical::Token *op, Expr *base, Lexical::Token *member);
BinaryExpr*  makeAssignment(Lexical::Token *assignOp, Expr *lvalue, Expr *assignValue);

TernaryExpr* makeTernary(Lexical::Token *tok, Expr *cond, Expr *yes, Expr *no);

CallExpr*    makeCall(FuncDecl *func, Utils::ExprList&&);
} // namespace Semantic
} // namespace Compiler

#endif // EXPR_HPP
