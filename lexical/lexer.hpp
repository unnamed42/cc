#ifndef LEXER_HPP
#define LEXER_HPP

#include "common.hpp"
#include "text/stream.hpp"

namespace Compiler {

namespace Text {
class UChar;
class UString;
}

namespace Lexical { 

struct Token;

enum TokenType : uint32_t;

class Lexer {
    NO_COPY_MOVE(Lexer);
    private:
        /**< text source. */
        Text::Stream m_src;
        /**< beginning position of current parsing token. */
        long         m_col;
    public:
        /**
         * Initialize m_src with file stream, using a file located at given path.
         * @param path path to a file
         */
        Lexer(const char *path);
        
        /**
         * Initialize m_src with string stream, using the given string.
         * The given string's lifetime should be at least as long as this lexer.
         * @param src text string source
         */
        Lexer(const Text::UString &src);
        
        /**
         * Extract a token.
         * @return a token object.
         */
        Token* get();
    private:
        /**
         * Extract a token that is a preprocessing number.
         * @param ch the first character of this token
         * @return a token object.
         */
        Token* getNumber(Text::UChar ch);
        
        /**
         * Extract a token that is an identifier.
         * @param ch the first character of this token
         * @return a token object.
         */
        Token* getIdentifier(Text::UChar ch);
        
        /**
         * Extract a token that is a character.
         * Before executing this function, a '\'' must have been extracted.
         * @return a token object.
         */
        Token* getChar();
        
        /**
         * Extract a token that is a string.
         * Before executing this function, a '\"' must have been extracted.
         * @return a token object.
         */
        Token* getString();
        
        /**
         * Extract a unicode codepoint.
         * @param len length of this codepoint
         * @return the codepoint.
         */
        Text::UChar getUCN(unsigned len);
        
        /**
         * Get a character escape sequence.
         * Before executing this function, a '\\' must have been extracted.
         * @return real value of corresponding escape sequence.
         */
        Text::UChar getEscapedChar();
        
        /**
         * Get an escaped character encoded as hexadecimal or octal
         * @param ch the first character of that sequence
         * @return real value of that escaped character.
         */
        Text::UChar getHexChar();
        Text::UChar getOctChar(Text::UChar ch);
        
        /**
         * Extract a digraph.
         * @param ch the first character of this digraph
         * @return a token object.
         */
        Token* getDigraph(Text::UChar ch);
        
        /**
         * Construct a token with given token type attribute.
         * @param type a unsigned 32-bit integer from enumerator `TokenType`
         * @param content content of this token
         * @return a constructed token object
         */
        Token* makeToken(TokenType type);
        Token* makeToken(TokenType type, const Text::UString &content);
        
        /**
         * Record new token's beginning position
         */
        void logPos();
        
        const SourceLoc* sourceLoc() const noexcept;
};
} // namespace Lexical

} // namespace Compiler

#endif // LEXER_HPP