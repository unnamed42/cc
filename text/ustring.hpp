#ifndef USTRING_HPP
#define USTRING_HPP

namespace Compiler {
namespace Text { 

class UChar;

class UString {
    public:
        class Iterator;
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
        
        ~UString() noexcept;
        
        void swap(UString &other) noexcept;
        
        void reserve(unsigned size);
        
        unsigned size() const noexcept;
        unsigned capacity() const noexcept;
        
        bool empty() const noexcept;
        
        void pushBack(UChar);
        void popBack() noexcept;
        
        Iterator begin() noexcept;
        Iterator end() noexcept;
        
        UChar& front() noexcept;
        UChar  front() const noexcept;
        UChar& back() noexcept;
        UChar  back() const noexcept;
        
        self& append(char ch);
        self& append(UChar ch);
        self& append(const char *str);
        self& append(const self &other);
        
        UChar& operator[](unsigned index) noexcept;
        UChar  operator[](unsigned index) const noexcept;
        
        self& operator+=(UChar);
        self& operator+=(const self&);
        self  operator+(UChar) const;
        self  operator+(const self&) const;
        
        self& operator=(self other);
    private:
        void resize(unsigned size);
};

} // namespace Text
} // namespace Compiler

#endif // USTRING_HPP
