#include "text/file.hpp"
#include "text/uchar.hpp"

#include <cctype>
#include <cassert>
#include <climits>

namespace impl = Compiler::Text;

using namespace Compiler::Text;

static constexpr auto SHIFT = sizeof(char) * CHAR_BIT;
static constexpr UChar::ValueType INVALID = 0xffffffff;

template <class T>
class InputSource {
    private:
        T& source;
    public:
        InputSource(T &s) : source(s) {}
        char get();
};

template <> char InputSource<File>::get() { return source.read(); }
template <> char InputSource<const char*>::get() { return *(source++); }

template <class T>
static UChar::ValueType fromInputSource(InputSource<T> source) {
    UChar::ValueType ch;
    unsigned char ch_ = source.get();
    
    if(!ch_) 
        return INVALID;
    auto bytes = validUTF8Head(ch_);
    if(!bytes)
        throw EncodingException{"bad UTF8 first byte"};
    addUTF8Byte(ch, ch_);
    
    while(--bytes) {
        ch_ = source.get();
        if(!ch_ || !validUTF8Byte(ch_)) 
            throw EncodingException{"bad UTF8 byte"};
        addUTF8Byte(ch, ch_);
    }
    
    return ch;
}

static inline UChar::ValueType fromFile(File &file) {
    return fromInputSource(InputSource<File>{file});
}

static inline UChar::ValueType fromByteArray(const char *str) {
    return fromInputSource(InputSource<const char*>{str});
}

bool impl::validUTF8Byte(char byte) noexcept {
    return (byte & 0xc0) != 0x80;
}

int impl::validUTF8Head(char byte) noexcept {
    switch(static_cast<unsigned char>(byte)) {
        // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        case 0xf0 ... 0xf7: return 4;
        // 1110xxxx 10xxxxxx 10xxxxxx
        case 0xe0 ... 0xef: return 3;
        // 110xxxxx 10xxxxxx
        case 0xc0 ... 0xdf: return 2;
        // 0xxxxxxx
        case 0x00 ... 0x7f: return 1;
        // invalid
        default:            return 0;
    }
}

int impl::UTF8Bytes(UChar::ValueType utf8) noexcept {
    int count = 1;
    while(utf8 >>= SHIFT)
        ++count;
    return count;
}

void impl::addUTF8Byte(UChar::ValueType &utf8, char byte) noexcept {
    utf8 <<= SHIFT;
    utf8 |= byte;
}

UChar UChar::makeInvalid() noexcept {
    return {INVALID};
}

UChar::UChar(ValueType ch) noexcept : m_ch(ch) {}

UChar::UChar(const UChar &other) noexcept : m_ch(other.m_ch) {}

UChar::UChar(File *file) : m_ch(fromFile(*file)) {}

UChar::UChar(const char *source) : m_ch(fromByteArray(source)) {}

const char* UChar::data() const noexcept {
    return m_data;
}

int UChar::bytes() const noexcept {
    return UTF8Bytes(m_ch);
}

void UChar::clear() noexcept {
    m_ch = 0;
}

bool UChar::invalid() const noexcept {
    return m_ch == INVALID;
}

bool UChar::isASCII() const noexcept {
    return m_ch <= 0x7f;
}

// 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
// 1110xxxx 10xxxxxx 10xxxxxx
// 110xxxxx 10xxxxxx
// 0xxxxxxx, ASCII
bool UChar::isUTF8() const noexcept {
    // first elements of these two arrays are sentinel value
    static unsigned masks[] = {0, 0x80, 0xe0, 0xf0, 0xf8};
    static unsigned prefixes[] = {1, 0, 0xc0, 0xe0, 0xf0};
    
    auto val = m_ch;
    
    unsigned count = 0;
    while(validUTF8Byte(val & 0xff)) {
        val >>= SHIFT;
        ++count;
    }
    
    return (val & masks[count]) == prefixes[count];
}

bool UChar::isNewline() const noexcept {
#ifdef MACHINTOSH
    return ch == '\r';
#else
    return m_ch == '\n';
#endif
}

bool UChar::isSpace() const noexcept {
    return isspace(m_ch);
}

bool UChar::isAlpha() const noexcept {
    return isalpha(m_ch);
}

bool UChar::isNumber() const noexcept {
    return isdigit(m_ch);
}

bool UChar::isAlNum() const noexcept {
    return isalnum(m_ch);
}

bool UChar::isOct() const noexcept {
    return ('0' <= m_ch && m_ch <= '7');
}

bool UChar::isHex() const noexcept {
    return isxdigit(m_ch);
}

int UChar::toNumber() const noexcept {
    switch(m_ch) {
        case '0': return 0;
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        case '8': return 8;
        case '9': return 9;
        default:  return -1;
    }
}

int UChar::toOct() const noexcept {
    switch(m_ch) {
        case '0': return 0;
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        default:  return -1;
    }
}

int UChar::toHex() const noexcept {
    auto num = toNumber();
    if(num != -1)
        return num;
    switch(m_ch) {
        case 'a': case 'A': return 10;
        case 'b': case 'B': return 11;
        case 'c': case 'C': return 12;
        case 'd': case 'D': return 13;
        case 'e': case 'E': return 14;
        case 'f': case 'F': return 15;
        default: return -1;
    }
}

UChar UChar::toLower() const noexcept {
    return { static_cast<ValueType>(tolower(m_ch)) };
}

UChar UChar::toUpper() const noexcept {
    return { static_cast<ValueType>(toupper(m_ch)) };
}

UChar::operator ValueType() const noexcept {
    return m_ch;
}

UChar& UChar::operator=(UChar other) noexcept {
    m_ch = other.m_ch;
    return *this;
}
