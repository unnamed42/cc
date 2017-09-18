#ifndef UCHARTYPE_HPP
#define UCHARTYPE_HPP

#include <cstdint>
#include <exception>

namespace Compiler {
namespace Text { 

class EncodingException : public std::exception {
    private:
        const char *message;
    public:
        EncodingException(const char *msg) noexcept : message(msg) {}
        const char *what() const noexcept override { return message; }
};

class File;

class UChar {
    private:
        using self = UChar;
    private:
        union {
            uint32_t ch;
            char data[sizeof(ch)];
        };
    public:
        UChar(uint32_t ch = 0) noexcept;
        UChar(const UChar &other) noexcept;
        explicit UChar(File *source);
        
        int bytes() const noexcept;
        
        bool isASCII() const noexcept;
        bool isUTF8() const noexcept;
        
        bool isAlpha() const noexcept;
        bool isNumber() const noexcept;
        bool isOct() const noexcept;
        bool isHex() const noexcept;
        
        int toNumber() const noexcept;
        int toOct() const noexcept;
        int toHex() const noexcept;
        
        void toLower() noexcept;
        void toUpper() noexcept;
        
        operator uint32_t() const noexcept;
        
        self& operator=(self other) noexcept;
    private:
        self& append(char ch) noexcept;
};

} // namespace Text
} // namespace Compiler

#endif // UCHARTYPE_HPP
