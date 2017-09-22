#ifndef DECL_HPP
#define DECL_HPP

#include "utils/ptrlist.hpp"
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

class Decl {
    private:
        /**< declared name */
        Lexical::Token *m_tok;
        /**< declared type */
        QualType        m_type;
        /**< storage class specifier */
        StorageClass    m_stor;
    public:
        Decl(Lexical::Token*, QualType, StorageClass) noexcept;
        virtual ~Decl() = default;
        
        Lexical::Token* token() noexcept;
        const Text::UString& name() const noexcept;
        Diagnostic::SourceLoc* sourceLoc() noexcept;
        QualType type() noexcept;
        StorageClass storageClass() const noexcept;
        
        virtual FuncDecl* toFunc() noexcept;
};

class FuncDecl : public Decl {
    private:
        Utils::PtrList m_params;
    public:
        FuncDecl(Lexical::Token*, QualType, StorageClass stor, Utils::PtrList&&) noexcept;
        
        FuncDecl* toFunc() noexcept override;
        
        Utils::PtrList& params() noexcept;
};

Decl* makeDecl(Lexical::Token *name, QualType type, StorageClass stor = Auto);

} // namespace Semantic
} // namespace Compiler

#endif // DECL_HPP
