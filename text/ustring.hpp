#ifndef USTRING_HPP
#define USTRING_HPP

#include <cstdint>

namespace Compiler {
namespace Text {

class UChar;

class UString {
    friend class Buffer;
    private:
        using self = UString;
    private:
        /**< length of string */
        uint16_t m_slen;
        /**< length of data */
        uint16_t m_dlen;
        /**< hack to store data */
        uint8_t  m_data[0];
    public:
        /**
         * @return length of string
         */
        uint16_t length() const noexcept;
        
        /**
         * @return data in bytes
         */
        uint16_t dataLength() const noexcept;
        
        const uint8_t* data() const noexcept;
        
        /**
         * Create a new string with the new character appended at the end
         * @param ch character to be added
         */
        self& operator+(UChar ch) const;
        self& operator+(char ch) const;
        
        bool operator==(const self &other) const noexcept;
        bool operator!=(const self &other) const noexcept;
};

} // namespace Text
} // namespace Compiler

namespace std {
template <class T> struct hash;

template <>
struct hash<Compiler::Text::UString> {
    std::size_t operator()(const Compiler::Text::UString&) const;
};
}

#endif // USTRING_HPP
