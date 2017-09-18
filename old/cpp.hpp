#ifndef __COMPILER_PREPROCESSOR__
#define __COMPILER_PREPROCESSOR__

#include "lexer.hpp"

#include <string>
#include <unordered_set>
#include <unordered_map>

namespace compiler {

struct macro {};

// C PreProcessor
class cpp {
    public:
        typedef std::unordered_map<std::string, macro> macro_table;
        typedef std::unordered_set<std::string> hash_set;
    private:
        lexer m_lex;
        token_list m_buffer;
        token_list m_parsed;
        
        bool has_newline;
        // depth of nested if directive
        unsigned int if_depth;
    private:
        static hash_set include_dirs;
    private:
        // get a token, a helper function
        token* get_tok();
        // push into buffer
        void unget_tok(token*);
        
        void exec_directive();
        void exec_include();
        void exec_if();
        // true - #ifdef; false - #ifndef
        void exec_ifdef(bool);
        void exec_define();
        void exec_undef();
        void exec_line();
        //void exec_error();
        void exec_pragma();
        
        void add_macro();
        void expand_macro();
        
        // should be done during lexical analysis
        token* concat_string(token*);
    public:
        cpp();
        cpp(const char*);
        
        // the real get functions
        token* get();
        token* get(token_attr);
        token* peek();
        bool   peek(token_attr);
        void   ignore();
        void   unget(token*);
        
        bool expect(token_attr);
        bool test(token_attr);
        
        bool end() const;
        bool empty() const;
        
        cpp(const cpp&) = delete;
        cpp& operator=(const cpp&) = delete;
};

} // namespace compiler

#endif // __COMPILER_PREPROCESSOR__
