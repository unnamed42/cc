#ifndef FILE_HPP
#define FILE_HPP

#include "common.hpp"

#include <cstdio>

namespace Compiler {
namespace Text {

class UChar;
class UString;

class File {
    NO_COPY_MOVE(File);
    private:
        using self = File;
    private:
        FILE *m_file;
    public:
        explicit File(const char *path);
        explicit File(FILE *f);
        
        ~File();
        
        /**
         * Read one byte
         * @return read data from file
         */
        char read();
        
        /**
         * Peek one byte
         * @return peek data from file
         */
        char peekASCII();
        
        /**
         * Get one UTF8 character
         * @return character taken
         */
        UChar get();
        
        /**
         * Peek one UTF8 character, more expensive than get()
         * @return peeked character
         */
        UChar peek();
        
        /**
         * Unget one byte
         */
        void ungetASCII();
        
        /**
         * Unget bytes until the specified byte found. 
         * After unget, the next read byte is next to the found one.
         * @param byte the specified byte
         * @return bytes returned back
         */
        int ungetUntilASCII(char byte);
        
        /**
         * Unget one character
         * @return character ungotten
         */
        UChar unget();
        
        /**
         * Unget characters 
         * @param ch the specified character
         * @return bytes returned back, identical to ch.bytes()
         */
        int unget(UChar ch);
        
        /**
         * Unget characters until the specified character found. 
         * After unget, the next read character is next to the found one.
         * @param ch the specified character
         * @return bytes returned back
         */
        int ungetUntil(UChar ch);
        
        /**
         * Ignore one byte
         */
        void ignoreASCII();
        
        /**
         * Ignore bytes until the specified byte found
         * @param byte the specified byte, also get ignored
         * @return bytes ignored
         */
        int ignoreUntilASCII(char byte);
        
        /**
         * Ignore current character
         */
        void ignore();
        
        /**
         * Ignore as many bytes as ch have
         * @param ch hint character
         */
        void ignore(UChar ch);
        
        /**
         * Ignore characters until the specified character found
         * @param ch the specified character, also get ignored
         * @return bytes ignored
         */
        int ignoreUntil(UChar ch);
        
        void open(const char *path);
        void flush();
        void close();
        
        long tell();
        
        /**
         * Move reading cursor to the specified location
         * @param offset offset to move
         * @param origin SEEK_SET/SEEK_CUR/SEEK_END
         * @return true if success
         */
        bool seek(long offset, int origin = SEEK_CUR);
        
        bool eof();
        bool error();
        bool good();
        
        FILE* getFILE();
        
        explicit operator bool();
        
        self& operator>>(UChar &result);
        self& operator>>(UString &result);
};

} // namespace Text
} // namespace Compiler

#endif // FILE_HPP
