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
    private:
        Diagnostic::SourceLoc *m_loc;
        TokenType              m_type;
    public:
        Token() noexcept = default;
        Token(Diagnostic::SourceLoc*, TokenType) noexcept;
        virtual ~Token() = default;
        
        bool is(TokenType type) const noexcept;
        Diagnostic::SourceLoc* sourceLoc() noexcept;
        TokenType type() noexcept;
        
        virtual Text::UString toString() const;
};

class ContentToken : public Token {
    private:
        Text::UString *content_;
    public:
        using Token::Token;
        ContentToken(Diagnostic::SourceLoc*, TokenType, Text::UString*);
        
        Text::UString toString() const override;
        const Text::UString* content() const noexcept;
};

Token* makeToken(Diagnostic::SourceLoc *source, TokenType type);
Token* makeToken(Diagnostic::SourceLoc *source, TokenType type, Text::UString *str);

} // namespace Lexical
} // namespace Compiler

#endif // TOKEN_HPP
