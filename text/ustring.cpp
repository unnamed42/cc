#include "utils/mempool.hpp"
#include "text/uchar.hpp"
#include "text/ustring.hpp"

#include <new>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <iterator>

namespace impl = Compiler::Text;

using namespace Compiler;
using namespace Compiler::Text;
using namespace Compiler::Utils;

template <class T>
static inline void swap(T &a, T &b) {
    T temp{};
    temp = a; a = b; b = temp;
}

template <class T>
struct allocator {
    T* allocate(size_t size) {
        auto ret = static_cast<T*>(malloc(size * sizeof(T)));
        if(!ret)
            throw std::bad_alloc{};
        return ret;
    }
    
    void deallocate(T *mem, size_t size) {
        free(mem);
    }
    
    T* reallocate(T *mem, size_t newsize) {
        auto ret = static_cast<T*>(realloc(mem, newsize));
        if(!ret)
            throw std::bad_alloc{};
        return ret;
    }
};

static allocator<UChar> alloc{};

UString* impl::clone(UString &&str) {
    return new (pool.allocate(sizeof(UString))) UString{std::move(str)};
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

UString::UString() : m_str(alloc.allocate(16)), m_len(0), m_capacity(16) {}

UString::UString(const char *source) : UString() {
    append(source);
}

UString::UString(const self &other) 
    : m_str(alloc.allocate(other.m_capacity)), 
      m_len(other.m_len), m_capacity(other.m_capacity) {
    memcpy(m_str, other.m_str, other.m_len * sizeof(*m_str));
}

UString::UString(self &&other) noexcept 
    : m_str(other.m_str), m_len(other.m_len), m_capacity(other.m_capacity) {
    other.m_str = nullptr;
}

UString::UString(unsigned reserve) : UString() {
    this->reserve(reserve);
}

UString::UString(UChar c, unsigned count) : UString(count) {
    assert(count != 0);
    m_len = count;
    while(--count) 
        m_str[count] = c;
}

UString::~UString() noexcept {
    if(m_str)
        alloc.deallocate(m_str, m_capacity);
}

void UString::swap(UString &other) noexcept {
    ::swap(m_str, other.m_str);
    ::swap(m_len, other.m_len);
    ::swap(m_capacity, other.m_capacity);
}

void UString::resize(unsigned size) {
    m_capacity = size;
    m_str = alloc.reallocate(m_str, size);
}

void UString::reserve(unsigned size) {
    if(m_capacity < size) 
        resize(size);
}

void UString::clear() noexcept {
    m_len = 0;
}

bool UString::empty() const noexcept {
    return m_len == 0;
}

unsigned UString::size() const noexcept {
    return m_len;
}

unsigned UString::capacity() const noexcept {
    return m_capacity;
}

void UString::pushBack(UChar ch) {
    reserve(m_len + m_len/2);
    m_str[m_len++] = ch;
}

void UString::popBack() noexcept {
    --m_len;
}

UChar& UString::front() noexcept {
    return *m_str;
}

UChar UString::front() const noexcept {
    return *m_str;
}

UChar& UString::back() noexcept {
    return m_str[m_len - 1];
}

UChar UString::back() const noexcept {
    return m_str[m_len - 1];
}

UString::Iterator UString::begin() noexcept {
    return Iterator{m_str};
}

UString::Iterator UString::end() noexcept {
    return Iterator{m_str + m_len};
}

UString::ConstIterator UString::begin() const noexcept {
    return ConstIterator{Iterator{m_str}};
}

UString::ConstIterator UString::end() const noexcept {
    return ConstIterator{Iterator{m_str + m_len}};
}

UChar& UString::at(unsigned index) noexcept {
    return m_str[index];
}

UChar UString::at(unsigned index) const noexcept {
    return m_str[index];
}

UChar& UString::operator[](unsigned index) noexcept {
    return at(index);
}

UChar UString::operator[](unsigned index) const noexcept {
    return at(index);
}

UString& UString::append(UChar ch) {
    pushBack(ch);
    return *this;
}

UString& UString::append(const self &other) {
    reserve(m_capacity + other.m_len);
    memcpy(m_str + m_len, other.m_str, other.m_len * sizeof(*m_str));
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

UString& UString::operator+=(UChar ch) {
    return append(ch);
}

UString& UString::operator+=(const self &other) {
    return append(other);
}

UString& UString::operator+=(const char *str) {
    return append(str);
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

UString& UString::operator=(self other) {
    swap(other);
    return *this;
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
