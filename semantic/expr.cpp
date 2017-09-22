#include "utils/mempool.hpp"
#include "lexical/token.hpp"
#include "lexical/tokentype.hpp"
#include "semantic/expr.hpp"
#include "semantic/decl.hpp"
#include "semantic/type.hpp"
#include "semantic/opcode.hpp"
#include "semantic/qualtype.hpp"
#include "diagnostic/logger.hpp"

namespace impl = Compiler::Semantic;

using namespace Compiler::Utils;
using namespace Compiler::Lexical;
using namespace Compiler::Semantic;
using namespace Compiler::Diagnostic;

static Expr* tryCast(Expr *expr, QualType destType) {
    auto srcType = expr->type();
    
    auto srcPtr = srcType->toPointer();
    auto destPtr = destType->toPointer();
//     if(srcPtr && destPtr) {
//         if(srcPtr->isVoidPtr() || destPtr->isVoidPtr())
//             return expr;
//     }
//     if(!destType->isCompatible(srcType))
//         expr = makeCast(expr, destType);
    
    auto srcNumber = srcType->toNumber();
    auto destNumber = destType->toNumber();
    if(destNumber) {
        if(!srcNumber && !destNumber->isBool())
            derr << expr->sourceLoc() << "rhs is required to be an arithmetic type";
    } else if(destPtr) {
        srcType = srcType.decay();
        srcPtr = srcType->toPointer();
        if(!srcPtr)
            derr << expr->sourceLoc() 
                << "cannot cast type '" << expr->type() << "' to a pointer type";
        
        auto destBase = destPtr->base();
        auto srcBase = srcPtr->base();
        
        auto destQual = destBase.qual(), srcQual = srcBase.qual();
        if(~destQual & srcQual) 
            derr << expr->sourceLoc() << "the cast loses qualifier";
        else if(!destPtr->isCompatible(srcPtr) && !(destPtr->isVoidPtr() || srcPtr->isVoidPtr()))
            derr << expr->sourceLoc() << "cannot convert '" << srcType 
                << "' to type '" << destType << '\'';
    } else if(!destType->isCompatible(srcType))
        derr << expr->sourceLoc() << "cannot convert '" << srcType 
                << "' to type '" << destType << '\'';
    
    if(destType->isCompatible(srcType))
        return expr;
    return makeCast(expr, destType);
}

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

BinaryExpr* impl::makeMemberAccess(Token *op, Expr *base, Token *member) {
    OpCode access = op->is(Dot) ? OpMember : OpMemberPtr;
    auto baseType = base->type();
    
    auto basePtr = baseType->toPointer();
    
    if(access == OpMemberPtr) {
        if(!basePtr)
            derr << op->sourceLoc() << "a pointer type required";
        baseType = basePtr->base();
    } 
    if(!baseType->isComplete())
        derr << op->sourceLoc() << "invalid use of an incomplete type";
    
    auto baseStruct = baseType->toStruct();
    if(!baseStruct)
        derr << op->sourceLoc() << "a struct/union type required";
    
    auto members = baseStruct->members();
    auto id = members.begin(), end = members.end();
    while(id != end) {
        if((*id)->token()->content() == member->content()) 
            break;
        ++id;
    }
    
    if(id == end) 
        derr << member->sourceLoc() << member << " is not a member of struct/union " << base->type();
    
    /* C99 6.5.2.3 Structure and union members
     * 
     * If the first expression is a pointer to a qualified type, the result has 
     * the so-qualified version of the type of the designated member.
     */
    auto resType = (*id)->type();
    resType.addQual(baseType.qual());
    // FIXME: member expr 
    return new (pool.allocate(sizeof(BinaryExpr))) BinaryExpr(op, access, base, nullptr);
}

CallExpr* impl::makeCall(Token *tok, FuncDecl *func, Utils::ExprList &&args) {
    auto type = func->type();
    auto ptr = type->toPointer();
    if(ptr)
        type = ptr->base();
    
    auto funcType = type->toFunc();
    if(!funcType) 
        derr << tok->sourceLoc() 
            << "apply operator() to invalid type " << func->type();
    
    auto &params = func->params();
    
    auto param = params.begin(), paramEnd = params.end();
    auto arg = args.begin(), argEnd = args.end();
    
    for(; param != paramEnd; ++param, ++arg) {
        if(arg == argEnd) 
            derr << (*arg)->sourceLoc() << "too few arguments";
        *arg = tryCast(*arg, (*param)->type());
    }
    if(param == paramEnd && arg != argEnd && !funcType->isVaArgs())
        derr << (*arg)->sourceLoc() << "too many arguments";
    
    return new (pool.allocate(sizeof(CallExpr))) CallExpr(func, std::move(args));
}
