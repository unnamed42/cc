#ifndef FILE_HPP
#define FILE_HPP

#include "common.hpp"

#include <cstdio>
#include <cstdint>

namespace Compiler {
namespace Text {

class UChar;

class File {
    NO_COPY_MOVE(File);
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
        uint8_t read();
        
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
         * Unget characters
         * @return bytes moved backwards
         */
        int unget();
        int unget(UChar ch);
        
        /**
         * Ignore current character
         */
        void  ignore();
        
        /**
         * Ignore until given character met
         * @param ch end character, also taken
         * @return number of bytes taken
         */
        int ignore(UChar ch);
        
        void open(const char *path);
        void flush();
        void close();
        
        long tell();
        void seek(long offset, int origin);
        
        bool eof();
        bool error();
        
        FILE* getFILE();
        
        explicit operator bool();
};

} // namespace Text
} // namespace Compiler

#endif // FILE_HPP
