#ifndef PARSER_HPP
#define PARSER_HPP

#include "common.hpp"
#include "lexical/pp.hpp"

namespace Compiler {

namespace Diagnostic {
struct SourceLoc;
}
namespace Lexical {
class Token;
}

namespace Semantic {

enum StorageClass : uint32_t;

class Scope;

class Expr;
class ObjectExpr;
class ConstantExpr;

class QualType;
class StructType;
class FuncType;
class EnumType;
class ArrayType;

class Decl;

class Stmt;
class CondStmt;
class DeclStmt;
class JumpStmt;
class LabelStmt;
class CompoundStmt;

class Parser {
    NO_COPY_MOVE(Parser);
    private:
        Lexical::PP m_src;
        /**< current scope */
        Scope      *m_curr = nullptr;
        
        /**< jump labels used in loops/switch */
        LabelStmt *m_break = nullptr;
        LabelStmt *m_continue = nullptr;
    public:
        Parser(const char *path);
        
        void parse();
    private:
        Lexical::Token* get();
        Lexical::Token* peek();
        
        bool isSpecifier(Lexical::Token*);
        
        ObjectExpr* makeIdentifier(Lexical::Token*);
        
        Expr* primaryExpr();
        Expr* postfixExpr();
        Expr* assignmentExpr();
        Expr* expr();
        Expr* unaryExpr();
        Expr* castExpr();
        Expr* binaryExpr();
        Expr* binaryExpr(Expr*, unsigned precedence);
        Expr* conditionalExpr();
        Utils::ExprList argumentExprList();
        
        QualType typeName();
        
        /**
         * Parse a declaration-specifier according to grammar
         * @param type parsed type, returned by reference
         * @param stor optional storage-class specifier, nullptr means no storage is accepted
         * @param required true if a declaration-specifier is required here
         * @return true when parse succeeded
         */
        bool tryDeclSpecifier(QualType &type, StorageClass *stor = nullptr, bool required = false);
        
        // thin wrapper around tryDeclSpecifier()
        QualType typeSpecifier(StorageClass *stor = nullptr);
        // storage-class, type-specifier, type-qualifier list
        QualType declSpecifier(StorageClass&);
        
        StructType* structUnionSpecifier();
        void structDeclList(Utils::DeclList&);
        Decl* structDeclarator(QualType base);
        
        EnumType* enumSpecifier();
        void      enumeratorList();
        
        QualType pointer(QualType base);
        
        /**
         * Parses a function's parameter list
         * @param ret parsed return type of this function
         * @return type of parsed function
         */
        FuncType* paramTypeList(QualType ret);
        
        /**
         * Actions performed in declarator/abstract-declarator.
         * @param base parsed part of declarator, shall be completed
         * @param name name of this declarator, nullptr means it has no name
         */
        void tryDeclarator(QualType &base, Lexical::Token* &name);
        
        /**
         * Helper function of tryDeclarator().
         * @param base element type of array, or return type of function
         * @return parsed array type or function type
         */
        QualType arrayFuncDeclarator(QualType base);
        
        Decl* declarator(StorageClass stor, QualType base);
        Decl* directDeclarator(QualType base);
        
        QualType abstractDeclarator(QualType base);
        QualType directAbstractDeclarator(QualType);
        
        /**
         * Parse a declaration according to grammar
         * @param list where results go in
         * @param type type of declared variables
         * @param stor storage-class of declared variables
         */
        void declaration(Utils::StmtList &list, QualType type, StorageClass stor);
        
        void initDeclarators(Utils::StmtList &list, QualType type, StorageClass stor);
        
        Expr* initializer(QualType);
        Expr* arrayInitializer(ArrayType*);
        Expr* aggregateInitializer(StructType*);
        
        Stmt* statement();
        JumpStmt* jumpStatement();
        CondStmt* selectionStatement();
        CompoundStmt* labelStatement();
        CompoundStmt* compoundStatement(QualType func = nullptr);
        CompoundStmt* forLoop();
        CompoundStmt* whileLoop();
        CompoundStmt* doWhileLoop();
        
        /**
         * Parsing a function definition/implementation.
         * @param name name of function
         * @param func declared type of function
         * @param stor storage class specifiers of function
         * @return processed declaration of function
         */
        DeclStmt* functionDefinition(Lexical::Token *name, QualType func, uint32_t stor = 0);
        
        void translationUnit();
};

} // namespace Semantic
} // namespace Compiler

#endif // PARSER_HPP
