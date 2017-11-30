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
    public:
        using ValueType = uint32_t;
        class Iterator;
    private:
        union {
            ValueType m_ch;
            char m_data[sizeof(m_ch)];
        };
    public:
        static UChar makeInvalid() noexcept;
    public:
        UChar(ValueType ch = 0) noexcept;
        UChar(const UChar &other) noexcept;
        explicit UChar(File *source);
        explicit UChar(const char *source);
        
        const char* data() const noexcept;
        
        int bytes() const noexcept;
        
        bool invalid() const noexcept;
        
        bool isASCII() const noexcept;
        bool isUTF8() const noexcept;
        
        bool isNewline() const noexcept;
        bool isSpace() const noexcept;
        bool isAlpha() const noexcept;
        bool isNumber() const noexcept;
        bool isAlNum() const noexcept;
        bool isOct() const noexcept;
        bool isHex() const noexcept;
        
        int toNumber() const noexcept;
        int toOct() const noexcept;
        int toHex() const noexcept;
        
        self toLower() const noexcept;
        self toUpper() const noexcept;
        
        void clear() noexcept;
        
        operator ValueType() const noexcept;
        
        self& operator=(self other) noexcept;
};

/**
 * UTF8 trailing bytes validation
 * @param byte byte to validate
 * @return true if byte is in binary form 10xxxxxx
 */
int validUTF8Head(char byte) noexcept;

/**
 * UTF8 validation
 * @param byte first byte of UTF8 codepoint
 * @return number of byte this codepoint is, 0 if invalid
 */
bool validUTF8Byte(char byte) noexcept;

/**
 * Number of bytes of this UTF8 codepoint. 
 * If value is 0, then 1 is returned.
 * @return number of bytes of this UTF8 codepoint
 */
int UTF8Bytes(UChar::ValueType utf8) noexcept;

/**
 * Helper in forming a UTF8 codepoint, this function performs
 * no check on argument.
 * @param utf8 byte to be added to
 * @param byte byte to be added
 */
void addUTF8Byte(UChar::ValueType &utf8, char byte) noexcept;

} // namespace Text
} // namespace Compiler

#endif // UCHARTYPE_HPP
