#ifndef SCOPE_HPP
#define SCOPE_HPP

#include "text/ustring.hpp"
#include "semantic/typeenum.hpp"

#include <unordered_map>

namespace Compiler {

namespace Lexical {
class Token;
}

namespace Semantic {

/* C99 6.2.1 Scopes of identifiers
 * 
 * For each different entity that an identifier designates, the identifier is visible (i.e., can be
 * used) only within a region of program text called its scope. Different entities designated
 * by the same identifier either have different scopes, or are in different name spaces. There
 * are four kinds of scopes: function, file, block, and function prototype. (A function
 * prototype is a declaration of a function that declares the types of its parameters.)
 *
 * A label name is the only kind of identifier that has function scope. It can be used (in a
 * goto statement) anywhere in the function in which it appears, and is declared implicitly
 * by its syntactic appearance (followed by a : and a statement).
 *
 * Every other identifier has scope determined by the placement of its declaration (in a
 * declarator or type specifier). 
 *
 * If the declarator or type specifier that declares the identifier appears outside 
 * of any block or list of parameters, the identifier has file scope, which terminates 
 * at the end of the translation unit. 
 *
 * If the declarator or type specifier that declares the identifier appears inside
 * a block or within the list of parameter declarations in a function definition, 
 * the identifier has block scope, which terminates at the end of the associated block. 
 *
 * If the declarator or type specifier that declares the identifier appears within 
 * the list of parameter declarations in a function prototype (not part of a function
 * definition), the identifier has function prototype scope, which terminates at the end of the
 * function declarator. 
 *
 * If an identifier designates two different entities in the same name space, the scopes
 * might overlap. If so, the scope of one entity (the inner scope) will be a strict subset of 
 * the scope of the other entity (the outer scope). Within the inner scope, the identifier 
 * designates the entity declared in the inner scope; the entity declared in the outer
 * scope is hidden (and not visible) within the inner scope.
 *
 * ... 
 * 
 * Structure, union, and enumeration tags have scope that begins just after the appearance of
 * the tag in a type specifier that declares the tag. Each enumeration constant has scope that
 * begins just after the appearance of its defining enumerator in an enumerator list. Any
 * other identifier has scope that begins just after the completion of its declarator.
 */
enum ScopeType {
    FunctionScope,
    FileScope,
    BlockScope,
    ProtoScope,
};

enum StorageClass : uint32_t;

class Decl;

class QualType;

class Scope {
    private:
        using self = Scope;
    private:
        ScopeType m_type;
        Scope    *m_par;
        std::unordered_map<Text::UString, Decl*>
                  m_table;
    public:
        explicit Scope(ScopeType = BlockScope, Scope *parent = nullptr);
        
        bool is(ScopeType) const noexcept;
        
        ScopeType type() const noexcept;
        
        Decl* find(Lexical::Token *name, bool recursive = true);
        Decl* findTag(Lexical::Token *name, bool recursive = true);
        
        Decl* declare(Decl *decl);
        Decl* declareTag(Decl *decl);
    private:
        Decl* find(const Text::UString &name, bool recursive = true);
        Decl* declare(const Text::UString &name, Decl *decl);
};

} // namespace Semantic
} // namespace Compiler

#endif // SCOPE_HPP
