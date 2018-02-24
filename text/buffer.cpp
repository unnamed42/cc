#include "text/buffer.hpp"
#include "text/uchar.hpp"
#include "text/ustring.hpp"
#include "utils/mempool.hpp"

using namespace Compiler::Text;
using namespace Compiler::Utils;

Buffer::Buffer() {
    clear();
}

Buffer::~Buffer() noexcept {
    operator delete[](m_str, m_cap, pool);
}

void Buffer::append(UChar ch) {
    int count = 0;
    UChar::ValueType val = ch;
    
    while(!(val & 0xff000000)){
        val <<= 8;
        ++count;
    }
    count = 4 - count; // bytes occupied by ch
    if(m_str->m_dlen + count + sizeof(UString) > m_cap)
        resize();
    
    while(val) {
        m_str->m_data[m_str->m_dlen++] = static_cast<uint8_t>(val >> 24);
        val <<= 8;
    }
    
    ++m_str->m_slen;
}

UString* Buffer::finish() {
    auto ret = m_str;
    clear();
    return ret;
}

void Buffer::clear() {
    static constexpr auto init_len = 8;
    
    void *mem = new (pool) uint8_t[init_len];
    m_cap = init_len;
    m_str = static_cast<UString*>(mem);
    m_str->m_slen = m_str->m_dlen = 0;
}

void Buffer::resize() {
    auto newsize = m_cap + m_cap/2;
    m_str = static_cast<UString*>(pool.reallocate(m_str, m_cap, newsize));
    m_cap = newsize;
}
