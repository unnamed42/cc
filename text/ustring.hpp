#ifndef USTRING_HPP
#define USTRING_HPP

#include "utils/iterator.hpp"

namespace Compiler {
namespace Text { 

class UChar;

class UString {
    public:
        using Iterator = Utils::Iterator<UChar*>;
        using ConstIterator = Utils::ConstIterator<Iterator>;
    private:
        using self = UString;
    private:
        UChar   *m_str;
        unsigned m_len;
        unsigned m_capacity;
    public:
        static UString fromUnsigned(unsigned i);
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
