#include "scope.hpp"
#include "mempool.hpp"

using namespace compiler;

static mempool<scope> scope_pool{};

static uint32_t anony_tag = 1;

static std::string tagged(token *tok) {
    return std::string{tok->to_string()} += '@';
}

ast_ident* scope::find_current(const std::string &name) {
    auto it = m_table.find(name);
    return it == m_table.end() ? nullptr : it->second;
}

ast_ident* scope::find(const std::string &name) {
    auto res = find_current(name);
    return res ? res : (m_par ? m_par->find(name) : nullptr);
}

ast_ident* scope::find_current(token *tok) {
    auto res = find_current(tok->to_string());
    return res ? res : nullptr;
}

ast_ident* scope::find(token *tok) {
    auto res = find(tok->to_string());
    return res ? res : nullptr;
}

ast_ident* scope::find_tag(token *tok) {
    return find(tagged(tok));
}

ast_ident* scope::find_tag_current(token *tok) {
    return find_current(tagged(tok));
}

void scope::insert(ast_object *obj) {
    std::string name{};
    if(obj->m_tok) name = obj->m_tok->to_string();
    m_table.insert({std::move(name), obj});
}

ast_object* scope::declare(token *tok, qual_type tp, uint8_t stor) {
    std::string name;
    uint32_t _anony = 0;
    if(!tok) {
        _anony = anony_tag++;
        name = "Anony[" + std::to_string(_anony) + ']';
    } else 
        name = tok->to_string();
    
    auto it = m_table.find(name);
    if(it != m_table.end())
        error(tok, "\"%s\" is already declared", name.c_str());
    
    auto obj = make_object(tok, tp, nullptr, stor, _anony);
    m_table.insert({std::move(name), obj});
    obj->decl = make_decl(obj);
    
    return obj;
}

ast_enum* scope::declare(token *tok, int val) {
    auto name = tok->to_string();
    auto it = m_table.find(name);
    if(it != m_table.end())
        error(tok, "\"%s\" is already declared", name);
    
    auto &&pair = m_table.insert({name, make_enum(tok, val)});
    return reinterpret_cast<ast_enum*>(pair.first->second);
}

ast_func* scope::declare_func(token *tok, qual_type tp, uint8_t stor, stmt_compound *body) {
    if(m_kind != FILE_SCOPE && m_kind != PROTO_SCOPE)
        error(tok, "Functions can only be declared in file or prototype scope");
    
    auto name = tok->to_string();
    
    auto it = m_table.find(name);
    if(it != m_table.end()) 
        error(tok, "\"%s\" is already declared", name);
    auto func = make_func(tok, tp, nullptr, stor, body);
    m_table.insert({name, func});
    func->decl = make_decl(func);
    
    return func;
}

ast_ident* scope::declare_tag(token *tok, type *tp) {
    auto name = tagged(tok);
    
    auto it = m_table.find(name);
    if(it != m_table.end())
        error(tok, "\"%s\" is already declared as a tag", tok->to_string());
    
    auto &&pair = m_table.insert({std::move(name), make_ident(tok, make_qual(tp))});
    return pair.first->second;
}


scope* compiler::make_scope(scope *par, scope_kind k) {
    return new (scope_pool.malloc()) scope(par, k);
}
