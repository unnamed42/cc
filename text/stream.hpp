#ifndef STREAM_HPP
#define STREAM_HPP

#include "text/file.hpp"
#include "diagnostic/sourceloc.hpp"

namespace Compiler {
namespace Text {

class UChar;
class UString;

class Stream {
    NO_COPY_MOVE(Stream);
    private:
        using SourceLoc = Diagnostic::SourceLoc;
        using self      = Stream;
    public:
        using PosType   = typename SourceLoc::PosType;
    private:
        /**< underlying text file */
        File      m_file;
        /**< source information of this stream */
        SourceLoc m_loc;
//         /**< last peeked character. If not read yet, its value should be `invalid`. */
//         UChar     m_peek;
    private:
        /**
         * Used for changing state after meeting a line break.
         */
        void newline() noexcept;
    public:
        explicit Stream(const char *path);
        
        UChar get() noexcept;
        
        /**
         * Peek the next character
         * @return the next character
         * @exception throws exception when encountered an invalid character
         */
        UChar peek();
        
        /**
         * check if next character matches the given one
         * @param ch the given character
         * @return true if matched
         */
        bool want(UChar ch) noexcept;
        
        void unget() noexcept; 
        
        /**
         * Ignore until given character met.
         * @param ch the given character, also get ignored.
         */
        void ignore(UChar ch) noexcept;
        
        UString getline();
        
        void skipLine() noexcept;
        
        /**
         * Before execute this function, a /* must have been extracted already.
         */
        void skipBlockComment() noexcept;
        
        /**
         * Skips whitespaces, single-line and multi-line comments
         * @return 0b00 when no spaces skipped
         *         0b01 when only spaces skipped
         *         0b10 when only line breaks skipped
         *         0b11 when both spaces and line breaks skipped
         */
        int skipSpace() noexcept;
        
        const SourceLoc& sourceLoc() const noexcept;
        
        /**
         * @return current reading position
         */
        PosType pos();
        
        const char *path() const noexcept;
        
        unsigned line() const noexcept;
        
        unsigned column() const noexcept;
        
        PosType lineBegin() const noexcept;
        
        explicit operator bool();
        self& operator>>(UChar &result);
};

} // namespace Text
} // namespace Compiler

#endif // STREAM_HPP
