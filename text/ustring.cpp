#include "utils/mempool.hpp"
#include "text/ustring.hpp"
#include "text/uchar.hpp"

#include <cstring>
#include <functional>

using namespace Compiler::Text;
using namespace Compiler::Utils;

uint16_t UString::length() const noexcept {
    return m_slen;
}

uint16_t UString::dataLength() const noexcept {
    return m_dlen;
}

const uint8_t* UString::data() const noexcept {
    return m_data;
}

UString& UString::operator+(UChar ch) const {
    int count = 0;
    UChar::ValueType val = ch;
    
    while(!(val & 0xff000000)){
        val <<= 8;
        ++count;
    }
    count = 4 - count; // bytes occupied by ch
    
    void *mem = new (pool) uint8_t[m_dlen + count];
    auto ret = static_cast<UString*>(mem);
    memcpy(ret, this, sizeof(UString) + m_dlen);
    
    while(val) {
        ret->m_data[ret->m_dlen++] = static_cast<uint8_t>(val >> 24);
        val <<= 8;
    }
    ++ret->m_slen;
    
    return *ret;
}

UString& UString::operator+(char ch) const {
    void *mem = new (pool) uint8_t[m_dlen + 1];
    auto ret = static_cast<UString*>(mem);
    memcpy(ret, this, sizeof(UString) + m_dlen);
    
    ret->m_data[ret->m_dlen++] = ch;
    ++ret->m_slen;
    
    return *ret;
}

bool UString::operator==(const self &o) const noexcept {
    return m_dlen == o.m_dlen && m_slen == o.m_slen && !memcmp(m_data, o.m_data, m_dlen);
}

bool UString::operator!=(const self &o) const noexcept {
    return !operator==(o);
}

std::size_t std::hash<UString>::operator()(const UString &str) const {
    size_t result = 0;
    constexpr size_t prime = 31;
    auto data = str.data();
    int len = str.dataLength();
    for (int i = 0; i < len; ++i) {
        result *= prime;
        result += data[i];
    }
    return result;
}
