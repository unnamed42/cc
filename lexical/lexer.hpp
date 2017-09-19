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

class Token;

enum TokenType : uint32_t;

class Lexer {
    NO_COPY_MOVE(Lexer);
    private:
        using Stream    = Text::Stream;
        using UChar     = Text::UChar;
        using UString   = Text::UString;
        using SourceLoc = Diagnostic::SourceLoc;
        using PosType   = typename SourceLoc::PosType;
    private:
        /**< text source. */
        Stream  m_src;
        /**< beginning position of current parsing token. */
        PosType m_pos;
    public:
        /**
         * Initialize m_src with file stream, using a file located at given path.
         * @param path path to a file
         */
        Lexer(const char *path);
        
//         /**
//          * Initialize m_src with string stream, using the given string.
//          * The given string's lifetime should be at least as long as this lexer.
//          * @param src text string source
//          */
//         Lexer(const UString &src);
        
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
        Token* getNumber(UChar ch);
        
        /**
         * Extract a token that is an identifier.
         * @param ch the first character of this token
         * @return a token object.
         */
        Token* getIdentifier(UChar ch);
        
        /**
         * Extract a token that is a character.
         * Before executing this function, a ' must have been extracted.
         * @return a token object.
         */
        Token* getChar();
        
        /**
         * Extract a token that is a string.
         * Before executing this function, a " must have been extracted.
         * @return a token object.
         */
        Token* getString();
        
        /**
         * Extract a unicode codepoint.
         * @param len length of this codepoint
         * @return the codepoint.
         */
        UChar getUCN(unsigned len);
        
        /**
         * Get a character escape sequence.
         * Before executing this function, a \ must have been extracted.
         * @return real value of corresponding escape sequence.
         */
        UChar getEscapedChar();
        
        /**
         * Get an escaped character encoded as hexadecimal
         * @return real value of that escaped character.
         */
        UChar getHexChar();
        
        /**
         * Get an escaped character encoded as octal
         * @param ch the first character of that sequence
         * @return real value of that escaped character.
         */
        UChar getOctChar(UChar ch);
        
        /**
         * Extract a digraph.
         * @param ch the first character of this digraph
         * @return a token object.
         */
        Token* getDigraph(UChar ch);
        
        /**
         * Construct a token with given token type attribute.
         * @param type a unsigned 32-bit integer from enumerator `TokenType`
         * @return a constructed token object
         */
        Token* makeToken(TokenType type);
        
        /**
         * Construct a token with given token type attribute.
         * @param type a unsigned 32-bit integer from enumerator `TokenType`
         * @param content content of this token
         * @return a constructed token object
         */
        Token* makeToken(TokenType type, UString &content);
        
        /**
         * Record new token's beginning position
         */
        void logPos();
        
        const SourceLoc* sourceLoc() const noexcept;
};

} // namespace Lexical
} // namespace Compiler

#endif // LEXER_HPP
