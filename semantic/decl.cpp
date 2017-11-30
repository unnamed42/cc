#include "utils/mempool.hpp"
#include "text/ustring.hpp"
#include "lexical/token.hpp"
#include "semantic/decl.hpp"
#include "semantic/type.hpp"

namespace impl = Compiler::Semantic;

using namespace Compiler::Text;
using namespace Compiler::Utils;
using namespace Compiler::Lexical;
using namespace Compiler::Semantic;
using namespace Compiler::Diagnostic;

Decl::Decl(Token *tok, QualType type, StorageClass stor) noexcept : m_tok(tok), m_type(type), m_stor(stor) {}
FuncDecl* Decl::toFuncDecl() noexcept { return nullptr; }
EnumDecl* Decl::toEnumDecl() noexcept { return nullptr; }
Token* Decl::token() noexcept { return m_tok; }
const SourceLoc* Decl::sourceLoc() const noexcept { return m_tok->sourceLoc(); }
QualType Decl::type() noexcept { return m_type; }
const UString& Decl::name() const noexcept { return *m_tok->content(); }
StorageClass Decl::storageClass() const noexcept { return m_stor; }
bool Decl::isType() const noexcept { return m_stor == Typedef; }
void Decl::setInit(Expr *expr) noexcept { m_init = expr; }

FuncDecl::FuncDecl(Token *tok, QualType type, StorageClass stor, DeclList &&params) noexcept 
    : Decl(tok, type, stor), m_params(move(params)) {}
FuncDecl* FuncDecl::toFuncDecl() noexcept { return this; }
DeclList& FuncDecl::params() noexcept { return m_params; }

EnumDecl::EnumDecl(Token *tok, int value) noexcept : Decl(tok, makeNumberType(Int), Static), m_val(value) {}
EnumDecl* EnumDecl::toEnumDecl() noexcept { return this; }
int EnumDecl::value() const noexcept { return m_val; }

Decl* impl::makeDecl(Token *tok, QualType type, StorageClass stor) {
    return new (pool) Decl(tok, type, stor);
}

EnumDecl* impl::makeDecl(Token *tok, int value) {
    return new (pool) EnumDecl(tok, value);
}
