#ifndef EXPR_HPP
#define EXPR_HPP

#include "utils/vector.hpp"
#include "semantic/stmt.hpp"
#include "semantic/qualtype.hpp"

#include <cstdint>

namespace Compiler {

namespace Diagnostic {
struct SourceLoc;
}
namespace Lexical {
class Token;
}
namespace ConstExpr {
class Value;
}

namespace Semantic {

enum OpCode : uint32_t;

class QualType;

class FuncDecl;

class Expr : public Stmt {
    protected:
        Lexical::Token *m_tok;
        QualType        m_type;
    public:
        explicit Expr(Lexical::Token *) noexcept;
        Expr(Lexical::Token*, QualType) noexcept;
        virtual ~Expr() = default;
        
        Lexical::Token* token() noexcept;
        
        const Diagnostic::SourceLoc* sourceLoc() const noexcept;
        
        QualType type() const noexcept;
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
        FuncDecl       *func;
        Utils::ExprList args;
    public:
        CallExpr(FuncDecl *func, Utils::ExprList &&args) noexcept;
};

class ObjectExpr : public Expr {
    private:
        Decl *decl;
    public:
        ObjectExpr(Lexical::Token *tok, Decl*) noexcept;
};

class ConstantExpr : public Expr {
    private:
        ConstExpr::Value *val;
    public:
        ConstantExpr(Lexical::Token*, ConstExpr::Value*) noexcept;
};

ConstantExpr* makeString(Lexical::Token*);
ConstantExpr* makeChar(Lexical::Token*);
ConstantExpr* makeNumber(Lexical::Token*);
ConstantExpr* makeBool(Lexical::Token*);
ConstantExpr* makeSizeOf(Lexical::Token *opSizeOf, Expr *expr);
ConstantExpr* makeSizeOf(Lexical::Token *opSizeOf, Type *type);

/**
 * Create a lvalue object.
 * @param tok where the identifier is used
 * @param decl where the identifier is declared
 */
ObjectExpr* makeObject(Lexical::Token *tok, Decl *decl);

UnaryExpr*   makeUnary(Lexical::Token *tok, OpCode op, Expr *expr);

CastExpr*    makeCast(Expr *from, QualType to);

BinaryExpr*  makeBinary(Lexical::Token *tok, OpCode op, Expr *lhs, Expr *rhs);

/**
 * Make a member access expression.
 * @param op token where . or -> is
 * @param base base object
 * @param member name of member
 * @return constructed BinaryExpr object
 */
BinaryExpr*  makeMemberAccess(Lexical::Token *op, Expr *base, Lexical::Token *member);
BinaryExpr*  makeAssignment(Lexical::Token *assignOp, Expr *lvalue, Expr *assignValue);

TernaryExpr* makeTernary(Lexical::Token *tok, Expr *cond, Expr *yes, Expr *no);

/**
 * Construct a CallExpr object.
 * @param tok token where left parenthese of function call expression
 * @param func function designator
 * @param args argument expression list
 * @return constructed object
 */
CallExpr*    makeCall(Lexical::Token *tok, FuncDecl *func, Utils::ExprList &&args);
} // namespace Semantic
} // namespace Compiler

#endif // EXPR_HPP
