#ifndef TOKEN_HPP
#define TOKEN_HPP

#include "common.hpp"

namespace Compiler {

namespace Diagnostic {
struct SourceLoc;
class Logger;
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
        virtual ~Token() = default;
        
        Token(Diagnostic::SourceLoc *loc, TokenType type) noexcept
            : m_loc(loc), m_type(type) {}
        
        inline bool is(TokenType type) const noexcept { return m_type == type; }
        
        inline const Diagnostic::SourceLoc* sourceLoc() const noexcept { return m_loc; }
        
        inline TokenType type() noexcept { return m_type; }
        
        virtual void print(Diagnostic::Logger&) const;
        
        virtual const Text::UString* content() const noexcept { return nullptr; }
};

class ContentToken : public Token {
    private:
        Text::UString *m_content;
    public:
        using Token::Token;
        
        ContentToken(Diagnostic::SourceLoc *loc, TokenType type, Text::UString *content) noexcept 
            : Token(loc, type), m_content(content) {}
        
        const Text::UString* content() const noexcept override { return m_content; }
        
        void print(Diagnostic::Logger&) const override;
};

Token* makeToken(Diagnostic::SourceLoc *source, TokenType type);
Token* makeToken(Diagnostic::SourceLoc *source, TokenType type, Text::UString *str);

} // namespace Lexical
} // namespace Compiler

#endif // TOKEN_HPP
