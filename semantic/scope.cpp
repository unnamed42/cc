#include "lexical/token.hpp"
#include "semantic/decl.hpp"
#include "semantic/scope.hpp"
#include "diagnostic/logger.hpp"

namespace impl = Compiler::Semantic;

using namespace Compiler::Text;
using namespace Compiler::Utils;
using namespace Compiler::Lexical;
using namespace Compiler::Semantic;

static UString& taggedName(const UString &name) {
    return name + '+';
}

Scope::Scope(ScopeType type, Scope *parent) : m_type(type), m_par(parent) {}

bool Scope::is(ScopeType type) const noexcept {
    return m_type == type;
}

ScopeType Scope::type() const noexcept {
    return m_type;
}

Decl* Scope::find(const UString &name, bool recursive) {
    auto decl = m_table.find(name);
    
    if(decl == m_table.end()) {
        if(!recursive) return nullptr;
        auto parent = m_par;
        while(parent) {
            auto &&table = parent->m_table;
            decl = table.find(name);
            if(decl == table.end())
                parent = parent->m_par;
            else
                return decl->second;
        }
        return nullptr;
    } else
        return decl->second;
}

Decl* Scope::find(Token *name, bool recursive) {
    auto &&nameStr = *reinterpret_cast<ContentToken*>(name)->content();
    return find(nameStr, recursive);
}

Decl* Scope::findTag(Token *name, bool recursive) {
    auto &&nameStr = *reinterpret_cast<ContentToken*>(name)->content();
    return find(taggedName(nameStr), recursive);
}

Decl* Scope::declare(const UString &name, Decl *decl) {
    auto prevDecl = find(name, false);
    
    if(!prevDecl) 
        derr.at(decl) << "redefinition of " << name << '\n'
            << prevDecl->sourceLoc() << "first declared here";
    
    return m_table.emplace(name, decl).first->second;
}

Decl* Scope::declare(Decl *decl) {
    return declare(decl->name(), decl);
}

Decl* Scope::declareTag(Decl *decl) {
    return declare(taggedName(decl->name()), decl);
}
