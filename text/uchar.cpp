#include "text/file.hpp"
#include "text/uchar.hpp"

#include <cctype>
#include <climits>

using namespace Compiler::Text;

static void validate(char ch) {
    if((ch & 0xc0) != 0x80)
        throw EncodingException{"bad UTF8 bytes"};
}

static constexpr auto SHIFT = sizeof(char) * CHAR_BIT;

UChar::UChar(uint32_t ch) noexcept : ch(ch) {}

UChar::UChar(const UChar &other) noexcept : ch(other.ch) {}

UChar::UChar(File *file) {
    auto ch_ = file->read();
    append(ch_);
    switch(ch_) {
        // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        case 0xf0 ... 0xf7: ch_ = file->read(); validate(ch_); append(ch_);
        // 1110xxxx 10xxxxxx 10xxxxxx
        case 0xe0 ... 0xef: ch_ = file->read(); validate(ch_); append(ch_);
        // 110xxxxx 10xxxxxx
        case 0xc0 ... 0xdf: ch_ = file->read(); validate(ch_); append(ch_);
        default: break;
    }
}

int UChar::bytes() const noexcept {
    if(!ch)
        return 1;
    int count = 0;
    auto val = ch;
    while(val) {
        val >>= SHIFT;
        ++count;
    }
    return count;
}

UChar& UChar::append(char ch) noexcept {
    ch <<= SHIFT;
    ch |= ch;
    return *this;
}

bool UChar::isASCII() const noexcept {
    return ch <= 0x7f;
}

// 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
// 1110xxxx 10xxxxxx 10xxxxxx
// 110xxxxx 10xxxxxx
// 0xxxxxxx, ASCII
bool UChar::isUTF8() const noexcept {
    // first elements of these two arrays are sentinel value
    static unsigned masks[] = {0, 0x80, 0xe0, 0xf0, 0xf8};
    static unsigned prefixes[] = {1, 0, 0xc0, 0xe0, 0xf0};
    
    auto val = ch;
    
    unsigned count = 0, mask = 0xc0, expect = 0x80;
    while((val & mask) == expect) {
        val >>= SHIFT;
        ++count;
    }
    
    return (val & masks[count]) == prefixes[count];
}

bool UChar::isAlpha() const noexcept {
    return isalpha(ch);
}

bool UChar::isNumber() const noexcept {
    return '0' <= ch && ch <= '9';
}

bool UChar::isOct() const noexcept {
    return ('0' <= ch && ch <= '7');
}

bool UChar::isHex() const noexcept {
    return isNumber() ||
           ('a' <= ch && ch <= 'f') ||
           ('A' <= ch && ch <= 'F');
}

int UChar::toNumber() const noexcept {
    switch(ch) {
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
    switch(ch) {
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
    switch(ch) {
        case 'a': case 'A': return 10;
        case 'b': case 'B': return 11;
        case 'c': case 'C': return 12;
        case 'd': case 'D': return 13;
        case 'e': case 'E': return 14;
        case 'f': case 'F': return 15;
        default: return -1;
    }
}

void UChar::toLower() noexcept {
    ch = tolower(ch);
}

void UChar::toUpper() noexcept {
    ch = toupper(ch);
}

UChar::operator uint32_t() const noexcept {
    return ch;
}

UChar& UChar::operator=(UChar other) noexcept {
    ch = other.ch;
    return *this;
}
