#ifndef PP_HPP
#define PP_HPP

#include "common.hpp"
#include "utils/ptrlist.hpp"
#include "lexical/lexer.hpp"

namespace Compiler {

namespace Lexical {

class Token;
enum TokenType : uint32_t;

/**
 * Preprocessor
 */
class PP {
    NO_COPY_MOVE(PP);
    private:
        Utils::TokenList m_unget;
        Lexer            m_src;
    public:
        explicit PP(const char *path);
        
        Token* get();
        
        Token* peek();
        
        bool peek(TokenType);
        
        void unget(Token*);
        
        Token* want(TokenType);
        Token* want(bool(*)(TokenType));
        
        bool nextIs(TokenType);
        
        void expect(TokenType);
};

} // namespace Lexical
} // namespace Compiler

#endif // PP_HPP
