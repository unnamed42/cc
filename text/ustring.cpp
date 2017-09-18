#include "text/uchar.hpp"
#include "text/ustring.hpp"

#include <memory>
#include <cstring>
#include <iterator>

using namespace Compiler::Text;

using std::allocator;

static allocator<UChar> alloc{};

template <class T>
static inline void swap(T &a, T &b) {
    T temp{};
    temp = a; a = b; b = temp;
}

class UString::Iterator {
    private:
        using traits_type = std::iterator_traits<UChar*>;
        using self        = UString::Iterator;
    public:
        using difference_type = traits_type::difference_type;
        using value_type      = traits_type::value_type;
        using pointer         = traits_type::pointer;
        using reference       = traits_type::reference;
    private:
        pointer m_cursor;
    public:
        explicit Iterator(pointer p) : m_cursor(p) {}
        Iterator(const self &other) : m_cursor(other.m_cursor) {}
        
        Iterator& operator++()    noexcept { ++m_cursor; return *this; }
        Iterator  operator++(int) noexcept { auto it = *this; ++m_cursor; return it; }
        Iterator& operator--()    noexcept { --m_cursor; return *this; }
        Iterator  operator--(int) noexcept { auto it = *this; --m_cursor; return it; }
        pointer   operator->()    noexcept { return m_cursor; }
        reference operator*()     noexcept { return *m_cursor; }
        
        Iterator& operator+=(difference_type diff) noexcept { m_cursor += diff; return *this; }
        Iterator  operator+(difference_type diff)  noexcept { return Iterator{m_cursor} += diff; }
        Iterator& operator-=(difference_type diff) noexcept { return *this += -diff; }
        Iterator  operator-(difference_type diff)  noexcept { return Iterator{m_cursor} -= diff; }
        
        bool operator==(const self &other) const noexcept { return m_cursor == other.m_cursor; }
        bool operator!=(const self &other) const noexcept { return !operator==(other); }
        bool operator<(const self &other)  const noexcept { return m_cursor < other.m_cursor; }
        bool operator>(const self &other)  const noexcept { return m_cursor > other.m_cursor; }
        bool operator<=(const self &other) const noexcept { return !operator>(other); }
        bool operator>=(const self &other) const noexcept { return !operator<(other); }
};

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
    auto mem = alloc.allocate(size);
    memcpy(mem, m_str, m_capacity * sizeof(*mem));
    alloc.deallocate(m_str, m_capacity);
    m_capacity = size;
    m_str = mem;
}

void UString::reserve(unsigned size) {
    if(m_capacity < size) 
        resize(size);
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

UChar& UString::operator[](unsigned index) noexcept {
    return m_str[index];
}

UChar  UString::operator[](unsigned index) const noexcept {
    return m_str[index];
}

UString& UString::append(char ch) {
    pushBack(UChar(ch));
    return *this;
}

UString& UString::append(const self &other) {
    reserve(m_capacity + other.m_len);
    memcpy(m_str + m_len, other.m_str, other.m_len * sizeof(*m_str));
    return *this;
}

UString& UString::append(const char *str) {
    
    return *this;
}

UString& UString::operator+=(UChar ch) {
    return append(ch);
}

UString& UString::operator+=(const self &other) {
    return append(other);
}

UString UString::operator+(UChar ch) const {
    return UString{*this} += ch;    
}

UString UString::operator+(const self &other) const {
    return UString{*this} += other;
}

UString& UString::operator=(self other) {
    swap(other);
    return *this;
}
