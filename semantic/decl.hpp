#ifndef DECL_HPP
#define DECL_HPP

#include "utils/vector.hpp"
#include "semantic/stmt.hpp"
#include "semantic/qualtype.hpp"
#include "semantic/typeenum.hpp"

namespace Compiler {

namespace Diagnostic {
struct SourceLoc;
}
namespace Text {
class UString;
}
namespace Lexical {
class Token;
}

namespace Semantic {

class FuncDecl;
class EnumDecl;

class Decl : public Stmt {
    protected:
        /**< declared name */
        Lexical::Token *m_tok;
        /**< declared type */
        QualType        m_type;
        /**< storage class specifier */
        StorageClass    m_stor;
        /**< initializer */
        Expr           *m_init = nullptr;
    public:
        Decl(Lexical::Token*, QualType, StorageClass) noexcept;
        virtual ~Decl() = default;
        
        Lexical::Token* token() noexcept;
        const Text::UString& name() const noexcept;
        const Diagnostic::SourceLoc* sourceLoc() const noexcept;
        QualType type() noexcept;
        StorageClass storageClass() const noexcept;
        
        bool isType() const noexcept;
        
        void setInit(Expr*) noexcept;
        
        virtual FuncDecl* toFuncDecl() noexcept;
        virtual EnumDecl* toEnumDecl() noexcept;
};

class FuncDecl : public Decl {
    private:
        Utils::DeclList  m_params;
        CompoundStmt    *m_body;
    public:
        FuncDecl(Lexical::Token*, QualType, StorageClass stor, Utils::DeclList&&) noexcept;
        
        FuncDecl* toFuncDecl() noexcept override;
        
        Utils::DeclList& params() noexcept;
        
        CompoundStmt* body() noexcept;
        void          setBody(CompoundStmt*) noexcept;
        
        void updateSignature(QualType) noexcept;
};

class EnumDecl : public Decl {
    private:
        int m_val;
    public:
        EnumDecl(Lexical::Token*, int val) noexcept;
        
        EnumDecl* toEnumDecl() noexcept override;
        
        int value() const noexcept;
};

Decl*     makeDecl(Lexical::Token *name, QualType type, StorageClass stor = Auto);
EnumDecl* makeDecl(Lexical::Token *name, int value);

} // namespace Semantic
} // namespace Compiler

#endif // DECL_HPP
