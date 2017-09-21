#include "utils/mempool.hpp"
#include "text/uchar.hpp"
#include "text/ustring.hpp"

#include <cstdlib>
#include <cassert>
#include <cstring>
#include <iterator>

namespace impl = Compiler::Text;

using namespace Compiler::Text;
using namespace Compiler::Utils;

template class Compiler::Utils::Vector<UChar>;

template <class T>
static inline void swap(T &a, T &b) {
    T temp{};
    temp = a; a = b; b = temp;
}

UString UString::fromUnsigned(unsigned i) {
    if(!i)
        return "0";
    UString ret{};
    while(i) {
        ret += static_cast<UChar>(i % 10 + '0');
        i /= 10;
    }
    return ret;
}

UString::UString(const char *source) : UString() {
    append(source);
}

UString& UString::append(char ch) {
    pushBack(UChar(ch));
    return *this;
}

UString& UString::append(UChar ch) {
    pushBack(ch);
    return *this;
}

UString& UString::append(const self &other) {
    reserve(m_cap + other.m_len);
    memcpy(m_data + m_len, other.m_data, other.m_len * sizeof(*m_data));
    return *this;
}

UString& UString::append(const char *str) {
    while(*str) {
        UChar ch{str};
        str += ch.bytes();
        pushBack(ch);
    }
    return *this;
}

UString& UString::operator+=(char ch) {
    return append(ch);
}

UString& UString::operator+=(UChar ch) {
    return append(ch);
}

UString& UString::operator+=(const self &other) {
    return append(other);
}

UString& UString::operator+=(const char *str) {
    return append(str);
}

UString UString::operator+(char ch) const {
    return UString{*this} += ch;
}

UString UString::operator+(UChar ch) const {
    return UString{*this} += ch;
}

UString UString::operator+(const self &other) const {
    return UString{*this} += other;
}

UString UString::operator+(const char *str) const {
    return UString{*this} += str;
}

bool UString::operator==(const UString &other) const noexcept {
    if(m_len != other.m_len)
        return false;
    for(auto i=0U; i<m_len; ++i) {
        if(at(i) != other.at(i))
            return false;
    }
    return true;
}

bool UString::operator!=(const UString &other) const noexcept {
    if(m_len != other.m_len)
        return true;
    for(auto i=0U; i<m_len; ++i) {
        if(at(i) == other.at(i))
            return true;
    }
    return false;
}
