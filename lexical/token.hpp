#ifndef TOKEN_HPP
#define TOKEN_HPP

#include "common.hpp"

namespace Compiler {
    
namespace Diagnostic {
struct SourceLoc;
}
    
namespace Lexical { 

enum TokenType : uint32_t;

class Token {
    private:
        using SourceLoc = Diagnostic::SourceLoc;
    private:
        SourceLoc *loc;
        TokenType  type;
    public:
        Token() = default;
        Token(SourceLoc*, TokenType);
        
        bool is(TokenType type) const noexcept;
};

Token* makeToken(Diagnostic::SourceLoc *source, TokenType type);

} // namespace Lexical
} // namespace Compiler

#endif // TOKEN_HPP
