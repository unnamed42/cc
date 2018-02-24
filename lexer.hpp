#ifndef __COMPILER_LEXER__
#define __COMPILER_LEXER__

#include "error.hpp" // file_pos
#include "token.hpp"

#include "concepts/non_copyable.hpp"

namespace compiler {

enum encoding: unsigned int {
    ASCII = 0, WCHAR = 1, CHAR16, CHAR32
};

class lexer: public non_copyable {
    public:
        typedef std::string string;
        typedef char32_t    char_t;
    private:
        const string *m_text; /**< text source of lexer */
        const char *m_pos;    /**< current reading position */
        file_pos m_loc;       /**< location to current reading file */
    private:
        token* make_token(char_t) const;
        
        char_t peek_helper();
    public:
        /**
         * @brief lexer empty initializer
         */
        lexer();
        
        /**
         * @brief initialize a lexer with given file location
         * @param location location of a file
         */
        lexer(const char *location);
        
        /**
         * @brief initialize a lexer with given string
         * @param text text source of this new lexer
         */
        lexer(const string &text);
        
        /**
         * @brief set line number
         * @param line line number to set
         */
        void set_line(unsigned line);
        
        /**
         * @brief get a single character. Trigraphs are handled here.
         *        current reading position gets advanced.
         * @return a UTF8 codepoint of current character.
         */
        char_t getc();
        
        
        /**
         * @brief peek a single character. Trigraphs are handled here.
         * @return a UTF8 codepoint of current character.
         */
        char_t peekc();
        
        /**
         * @brief put previously gotten character back.
         */
        void ungetc();
         
        /**
         * @brief ignore until meets the given character or EOF
         * @param ch the given character.
         * @param first_line if true, the given char must be the first 
         *        non-space of its line.
         */
        void ignore(char_t ch, bool first_line = false);
        
        /**
         * @brief get all until meets line break.
         * @return the gotten characters.
         */
        string getline();
        
        /**
         * @brief check if next is the given character, and consumes it when true
         * @param ch the given character
         * @return if matches
         */
        bool expect(char_t ch);
        
        token_list parse_line();
        
        bool end() const;
        bool empty() const;
        bool valid() const;
        
        // consumes the current line
        void skip_line();
        
        // skips a block comment, assume you have extracted /*
        void skip_block_comment();
        
        // skips whitespaces, single-line and multiline comments
        // returns true if a (lexical) newline is skipped
        bool skip_space();
        
        token* get();
        
        token* get_digraph(char_t);
        // int: first character
        token* get_number(char_t);
        // int: first character; encoding: encoding of first character
        token* get_identifier(char_t, encoding=ASCII);
        // encoding&: encoding of extracted escaped char
        char_t get_escaped_char(encoding&);
        //    \u hex-quad 
        // or \U hex-quad hex-quad
        char_t get_UCN(int);
        // int: first character
        char_t get_oct_char(char_t);
        char_t get_hex_char();
        // receives the text encoding
        token* get_char(encoding=ASCII);
        token* get_string(encoding=ASCII);
};

} // namespace compiler

#endif // __COMPILER_LEXER__
