#ifndef TOKEN_HPP
#define TOKEN_HPP

#include "common.hpp"
#include "sourceloc.hpp"

namespace Compiler {

namespace Lexical { 

enum TokenType : uint32_t;

struct Token {
    SourceLoc loc;
    TokenType type;
    
    bool is(TokenType type) const noexcept;
};

Token* makeToken(const SourceLoc *source, TokenType type);

} // namespace Lexical
} // namespace Compiler

#endif // TOKEN_HPP
