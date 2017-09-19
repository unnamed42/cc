#ifndef TOKEN_HPP
#define TOKEN_HPP

#include "common.hpp"

namespace Compiler {
    
namespace Diagnostic {
struct SourceLoc;
}

namespace Text {
class UString;
}

namespace Lexical { 

enum TokenType : uint32_t;

class Token {
    protected:
        using SourceLoc = Diagnostic::SourceLoc;
    private:
        SourceLoc *loc;
        TokenType  type;
    public:
        Token() = default;
        Token(SourceLoc*, TokenType);
        virtual ~Token() = default;
        
        bool is(TokenType type) const noexcept;
        virtual UString* content() { return nullptr; }
};

class ContentToken : public Token {
    private:
        using UString = Text::UString;
    private:
        UString *content_;
    public:
        using Token::Token;
        ContentToken(SourceLoc*, TokenType, UString&&);
        
        UString* content() override;
};

Token* makeToken(Diagnostic::SourceLoc *source, TokenType type);
Token* makeToken(Diagnostic::SourceLoc *source, TokenType type, Text::UString &&str);

} // namespace Lexical
} // namespace Compiler

#endif // TOKEN_HPP
