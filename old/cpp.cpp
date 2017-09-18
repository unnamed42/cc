#include "cpp.hpp"
#include "token.hpp"
#include "error.hpp"
#include "lexer.hpp"

#include <string>
#include <fstream>

using namespace compiler;

typedef std::string string;
typedef cpp::hash_set hash_set;
typedef cpp::macro_table macro_table;


static void merge_token(token*, token*);

// concat [pos, end)
static string op_to_string(token_list::iterator, token_list::iterator);

// pop front wrapper
static token* pop_front(token_list&);

static bool file_exists(const char*);
static string get_path(const string&);
static string search_file(const hash_set&, const string&);


hash_set cpp::include_dirs {
    "/usr/include/x86-64/gnu",
};

cpp::cpp():m_lex(), m_buffer(), m_parsed(), has_newline(false), if_depth(0) {}

cpp::cpp(const char *location):m_lex(location), m_buffer(), m_parsed(), has_newline(false), if_depth(0) {}

bool cpp::end() const {return m_lex.end() && m_buffer.empty() && m_parsed.empty();}

bool cpp::empty() const {return m_lex.empty() && m_buffer.empty() && m_parsed.empty();}

token* cpp::get_tok() {
    if(!m_buffer.empty()) return pop_front(m_buffer);
    auto tok = m_lex.get();
    switch(tok->m_attr) {
        case Newline: has_newline = true; return get_tok();
        case Eof: 
            if(if_depth) 
                error("In file %s:\nUnterminated conditional directive", tok->m_pos.m_name);
            // intended fall-through
        default: return tok;
    }
}

void cpp::unget_tok(token *tok) {m_buffer.emplace_front(tok);}

#ifdef CC_DEBUG
#include <iostream>
#endif
token* cpp::get() {
#ifdef CC_DEBUG
    auto tok = [this]() -> token* {
#endif
    if(!m_parsed.empty()) return pop_front(m_parsed);
    else if(empty()) return nullptr;
    
    auto tok = get_tok();
    
    switch(tok->m_attr) {
        case String: return concat_string(tok);
        case Pound: if(has_newline) {exec_directive(); return get();}
        default: 
            if(is_directive(tok->m_attr) && tok->m_attr != If && tok->m_attr != Else)
                tok->m_attr = Identifier;
            return tok;
    }
#ifdef CC_DEBUG
    }();
    std::cout<<"get: "<<tok->to_string()<<std::endl;
    return tok;
#endif 
}

token* cpp::get(token_attr attr) {
    auto tok = get();
    if(tok->m_attr != attr) {
        error(tok, "Expecting \"%s\", but get \"%s\"", attr_to_string(attr), tok->to_string());
        return nullptr;
    }
    return tok;
}

token* cpp::peek() {
#ifdef CC_DEBUG
    auto tok = [this]()->token*{
#endif

    if(!m_parsed.empty()) return m_parsed.front();
    
    auto tok = get();
    m_parsed.push_front(tok);
    return tok;
#ifdef CC_DEBUG
    }();
    std::cout<<"peek: "<<tok->to_string()<<std::endl;
    return tok;
#endif
}

bool cpp::peek(token_attr attr) {
    return peek()->m_attr == attr;
}

void cpp::ignore() {
    if(!m_parsed.empty()) m_parsed.pop_front();
    else get();
}

void cpp::unget(token *tok) {
#ifdef CC_DEBUG
    std::cout<<"unget: "<<tok->to_string()<<std::endl;
#endif
    m_parsed.push_front(tok);
}

bool cpp::expect(token_attr attr) {
    auto tok = get();
#ifdef CC_DEBUG
    std::cout<<"expect: "<<tok->to_string()<<std::endl;
#endif
    if(!tok->is(attr)) {
        error(tok, "Expecting \"%s\", but get \"%s\"", attr_to_string(attr), tok->to_string());
        return false;
    }
    return true;
} 

bool cpp::test(token_attr attr) {
    auto tok = get();
#ifdef CC_DEBUG
    std::cout<<"test: "<<tok->to_string()<<std::endl;
#endif
    if(!tok->is(attr)) {
        m_parsed.push_front(tok);
        return false;
    }
    return true;
}

token* cpp::concat_string(token* tok) {
    has_newline = false;
    for(;;) {
        auto _tok = get_tok();
        if(_tok && _tok->is(String)) 
            merge_token(tok, _tok);
        else 
            break;
    }
    return tok;
}

void cpp::exec_directive() {
    auto &&list = m_lex.parse_line();
    // get an empty line, re-get it
    if(list.empty()) {
        if(m_lex.valid()) exec_directive();
        else error("Unexpected end-of-file");
    }
    for(auto it = list.begin(); it != list.end(); ++it) {
        auto &&tok = *it;
        switch(tok->m_attr) {
            case Newline: error(tok, "A newline directly after #"); break;
            case DirectDefine: exec_define(); break;
            case DirectUndef: exec_undef(); break;
            case DirectInclude: exec_include(); break;
            case If: ++if_depth; exec_if(); break;
            case DirectElif: 
                if(!if_depth) error(tok, "Unexpected #elif");
                exec_if(); break;
            case Else:
                exec_if(); break;
            case DirectIfdef: exec_ifdef(true); break;
            case DirectIfndef: exec_ifdef(false); break;
            case DirectEndif: // Extra tokens are ignored
                if(!if_depth) error(tok, "#endif without #if");
                --if_depth; return;
            case DirectError: {
                auto str = op_to_string(++it, list.end());
                error(tok, "%s", str.c_str()); 
                return;
            }
            case DirectLine: exec_line(); break;
            case DirectPragma: warning(tok, "Unimplemented directive: #pragma"); return;
            default: error(tok, "Expecting a directive, but get %s", tok->to_string());
        }
    }
    has_newline = true;
}

void cpp::exec_define() {}

void cpp::exec_undef() {}

void cpp::exec_ifdef(bool required) {}

void cpp::exec_if() {}

void cpp::exec_line() {}

void cpp::exec_include() {
    static unsigned int include_depth = 0;
    ++include_depth;
    auto tok = get_tok();
    if(include_depth >= 50)
        error(tok, "File inclusion nested too deeply");
    std::string path{};
    if(tok->is(String)) { // #include "file"
        if(tok->m_str[0] != '/')
            path = get_path(tok->m_str) + tok->m_str;
    } else if(tok->is(LessThan)) { // #include <file>
        for(;;) {
            auto p = get_tok();
            if(p->is(GreaterThan)) break;
            path += p->to_string();
        }
        path = search_file(include_dirs, std::move(path));
    }
    
    cpp temp(path.c_str());
    // TODO: pass current marco table
    for(tok = temp.get(); tok;) {
        m_parsed.push_back(tok);
        tok = temp.get();
    }
    --include_depth;
}


void merge_token(token *lhs, token *rhs) {
    std::string str{lhs->to_string()};
    lhs->m_str = insert_string(str += rhs->to_string());
}

token* pop_front(token_list &l) {
    auto ptr = l.front();
    l.pop_front();
    return ptr;
}

string op_to_string(token_list::iterator pos, token_list::iterator end) {
    string result{};
    for(; pos != end; ++pos) 
        result += (*pos)->to_string();
    return result;
}

bool file_exists(const char *location) {
    std::ifstream test(location);
    return test.good();
}

string get_path(const string &src) {
    return src.substr(0,src.rfind('/'));
}

string search_file(const hash_set &set, const string &src) {
    string result{};
    for(auto &&dir: set) {
        result = dir + src;
        if(file_exists(result.c_str())) return result;
    }
    return src; // cannot be found
}
