#include "utils/mempool.hpp"
#include "lexical/token.hpp"
#include "semantic/decl.hpp"

namespace impl = Compiler::Semantic;

using namespace Compiler::Text;
using namespace Compiler::Utils;
using namespace Compiler::Lexical;
using namespace Compiler::Semantic;

Decl::Decl(Token *tok, QualType type, StorageClass stor) noexcept : m_tok(tok), m_type(type), m_stor(stor) {}
FuncDecl* Decl::toFunc() noexcept { return nullptr; }
Token* Decl::token() noexcept { return m_tok; }
QualType Decl::type() noexcept { return m_type; }
const UString& Decl::name() const noexcept { return *reinterpret_cast<ContentToken*>(m_tok)->content(); }

FuncDecl::FuncDecl(Token *tok, QualType type, StorageClass stor, PtrList &&params) noexcept 
    : Decl(tok, type, stor), m_params(static_cast<PtrList&&>(params)) {}
FuncDecl* FuncDecl::toFunc() noexcept { return this; }
PtrList& FuncDecl::params() noexcept { return m_params; }

Decl* impl::makeDecl(Token *tok, QualType type, StorageClass stor) {
    return new (pool.allocate<Decl>()) Decl(tok, type, stor);
}
