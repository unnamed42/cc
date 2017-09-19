#ifndef USTRING_HPP
#define USTRING_HPP

namespace Compiler {
namespace Text { 

class UChar;

class UString {
    public:
        class Iterator {
            private:
                using self = Iterator;
            public:
                using difference_type = std::ptrdiff_t;
                using value_type      = UChar;
                using pointer         = UChar*;
                using reference       = UChar&;
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
        class ConstIterator {
            private:
                using self = ConstIterator;
            public:
                using difference_type = std::ptrdiff_t;
                using value_type      = UChar;
                using pointer         = const UChar*;
                using reference       = const UChar&;
            private:
                pointer m_cursor;
            public:
                explicit ConstIterator(pointer p) : m_cursor(p) {}
                ConstIterator(const self &other) : m_cursor(other.m_cursor) {}
                
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
    private:
        using self = UString;
    private:
        UChar   *m_str;
        unsigned m_len;
        unsigned m_capacity;
    public:
        UString();
        UString(const char*);
        UString(const self&);
        UString(self&&) noexcept;
        UString(unsigned reserve);
        UString(UChar fill, unsigned count);
        
        ~UString() noexcept;
        
        void swap(self &other) noexcept;
        
        void clear() noexcept;
        void reserve(unsigned size);
        
        unsigned size() const noexcept;
        unsigned capacity() const noexcept;
        
        bool empty() const noexcept;
        
        void pushBack(UChar);
        void popBack() noexcept;
        
        Iterator begin() noexcept;
        Iterator end() noexcept;
        
        ConstIterator begin() const noexcept;
        ConstIterator end()   const noexcept;
        
        UChar& front() noexcept;
        UChar  front() const noexcept;
        UChar& back() noexcept;
        UChar  back() const noexcept;
        
        UChar& at(unsigned index) noexcept;
        UChar  at(unsigned index) const noexcept;
        
        UChar& operator[](unsigned index) noexcept;
        UChar  operator[](unsigned index) const noexcept;
        
        self& append(UChar ch);
        self& append(const char *str);
        self& append(const self &other);
        
        self& operator+=(UChar);
        self& operator+=(const self&);
        self& operator+=(const char*);
        self  operator+(UChar) const;
        self  operator+(const self&) const;
        self  operator+(const char*) const;
        
        self& operator=(self other);
        
        bool operator==(const self &other) const noexcept;
        bool operator!=(const self &other) const noexcept;
    private:
        void resize(unsigned size);
};

UString* clone(UString&&);

} // namespace Text
} // namespace Compiler

#endif // USTRING_HPP
