#ifndef __COMPILER_SCOPE__
#define __COMPILER_SCOPE__

#include "ast.hpp"

#include <string>
#include <cstdint>
#include <unordered_map>

namespace compiler {

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
enum scope_kind: uint8_t {
    FUNCTION_SCOPE = 0,
    FILE_SCOPE,
    BLOCK_SCOPE,
    PROTO_SCOPE, // prototype
};

class scope {
    public:
        typedef std::unordered_map<std::string, ast_ident*> table_t;
    private:
        scope *m_par; // outer scope
        scope_kind m_kind;
        
        table_t m_table;
    public:
        ast_ident* find(const std::string&);
        ast_ident* find_current(const std::string&);
    public:
        scope(scope *par, scope_kind k = BLOCK_SCOPE)
            :m_par(par), m_kind(k), m_table() {}
        
        ast_ident* find(token*);
        ast_ident* find_current(token*);
        
        ast_ident* find_tag(token*);
        ast_ident* find_tag_current(token*);
        
        ast_object* declare(token*, qual_type, uint8_t = 0);
        ast_enum*   declare(token*, int);
        ast_func*   declare_func(token*, qual_type, uint8_t = 0, stmt_compound* = nullptr);
        // a tag has no qualifier, use type* instead
        ast_ident*  declare_tag(token*, type*);
        
        uint8_t kind() const {return m_kind;}
        void    kind(scope_kind k) {m_kind = k;}
        
        void insert(ast_object*);
        
        table_t& table() {return m_table;}
};

scope* make_scope(scope* = nullptr, scope_kind = BLOCK_SCOPE);

} // namespace compiler

#endif // __COMPILER_SCOPE__
