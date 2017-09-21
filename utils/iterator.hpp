#ifndef ITERATOR_HPP
#define ITERATOR_HPP

namespace Compiler {
namespace Utils {

template <class> class Iterator;
template <class> class ConstIterator;

template <class T>
class Iterator<T*> {
    private:
        using self = Iterator<T*>;
    public:
        using difference_type = decltype((int*)(0) - (int*)(0));
        using value_type      = T;
        using pointer         = T*;
        using reference       = T&;
    private:
        pointer m_cursor;
    public:
        explicit Iterator(pointer p) : m_cursor(p) {}
        Iterator(const self &other) : m_cursor(other.m_cursor) {}

        self& operator++()     noexcept { ++m_cursor; return *this; }
        self  operator++(int)  noexcept { auto it = *this; ++m_cursor; return it; }
        self& operator--()     noexcept { --m_cursor; return *this; }
        self  operator--(int)  noexcept { auto it = *this; --m_cursor; return it; }
        pointer   operator->() noexcept { return m_cursor; }
        reference operator*()  noexcept { return *m_cursor; }

        self& operator+=(difference_type diff) noexcept { m_cursor += diff; return *this; }
        self  operator+(difference_type diff)  noexcept { return self{m_cursor} += diff; }
        self& operator-=(difference_type diff) noexcept { return *this += -diff; }
        self  operator-(difference_type diff)  noexcept { return self{m_cursor} -= diff; }

        bool operator==(const self &other) const noexcept { return m_cursor == other.m_cursor; }
        bool operator!=(const self &other) const noexcept { return !operator==(other); }
        bool operator<(const self &other)  const noexcept { return m_cursor < other.m_cursor; }
        bool operator>(const self &other)  const noexcept { return m_cursor > other.m_cursor; }
        bool operator<=(const self &other) const noexcept { return !operator>(other); }
        bool operator>=(const self &other) const noexcept { return !operator<(other); }
};

template <class T>
class ConstIterator<Iterator<T>> {
    private:
        using self = ConstIterator;
        using base = Iterator<T>;
    public:
        using difference_type = typename base::difference_type;
        using value_type      = typename base::value_type;
        using pointer         = const value_type*;
        using reference       = const value_type&;
    private:
        base m_cursor;
    public:
        explicit ConstIterator(const base &b) : m_cursor(b) {}
        explicit ConstIterator(value_type *p) : m_cursor(p) {}
        ConstIterator(const self &other) : m_cursor(other.m_cursor) {}
        
        base get() noexcept { return m_cursor; }
        
        self& operator++()     noexcept { ++m_cursor; return *this; }
        self  operator++(int)  noexcept { auto it = *this; ++m_cursor; return it; }
        self& operator--()     noexcept { --m_cursor; return *this; }
        self  operator--(int)  noexcept { auto it = *this; --m_cursor; return it; }
        pointer   operator->() noexcept { return m_cursor.operator->(); }
        reference operator*()  noexcept { return *m_cursor; }
        
        self& operator+=(difference_type diff) noexcept { m_cursor += diff; return *this; }
        self  operator+(difference_type diff)  noexcept { return self{m_cursor} += diff; }
        self& operator-=(difference_type diff) noexcept { return *this += -diff; }
        self  operator-(difference_type diff)  noexcept { return self{m_cursor} -= diff; }
        
        bool operator==(const self &other) const noexcept { return m_cursor == other.m_cursor; }
        bool operator!=(const self &other) const noexcept { return !operator==(other); }
        bool operator<(const self &other)  const noexcept { return m_cursor < other.m_cursor; }
        bool operator>(const self &other)  const noexcept { return m_cursor > other.m_cursor; }
        bool operator<=(const self &other) const noexcept { return !operator>(other); }
        bool operator>=(const self &other) const noexcept { return !operator<(other); }
};

} // namespace Utils
} // namespace Compiler

#endif // ITERATOR_HPP
