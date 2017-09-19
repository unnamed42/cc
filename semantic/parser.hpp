#ifndef PARSER_HPP
#define PARSER_HPP

#include "common.hpp"
#include "lexical/lexer.hpp"

namespace Compiler {

namespace Diagnostic {
struct SourceLoc;
}

namespace Semantic {

class Scope;

class Parser {
    NO_COPY_MOVE(Parser);
    
    private:
        using Lexer = Lexical::Lexer;
    private:
        Lexer  m_src;
        /**< current scope */
        Scope *m_curr;
        /**< file scope */
        Scope *m_file;
    public:
        Parser(const char *path);
        
        void parse();
};

extern Diagnostic::SourceLoc *epos;

} // namespace Semantic
} // namespace Compiler

#endif // PARSER_HPP
