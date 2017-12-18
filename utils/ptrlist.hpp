#ifndef PTRLIST_HPP
#define PTRLIST_HPP

#include "utils/iterator.hpp"

namespace Compiler {

namespace Lexical {
class Token;
}
namespace Semantic {
class Stmt;
class Expr;
class Decl;
}

namespace Utils {

class PtrListBase {
    private:
        using self = PtrListBase;
    protected:
        void   **m_data;
        unsigned m_len;
        unsigned m_cap;
    protected:
        explicit PtrListBase(unsigned reserve = 16);
        PtrListBase(const self&);
        PtrListBase(self&&) noexcept;
        ~PtrListBase() noexcept;
        
        void  pushBack(void *data);
        void* popBack() noexcept;
        
        void append(const self&);
        
        void swap(self &o) noexcept;
    
        bool     empty()    const noexcept { return m_len == 0; }
        unsigned size()     const noexcept { return m_len; }
        unsigned capacity() const noexcept { return m_cap; }
        
        void clear() noexcept { m_len = 0; }
        
        void reserve(unsigned);
};

template <class T>
class PtrList : private PtrListBase {
    private:
        using self = PtrList;
        using base = PtrListBase;
    public:
        class Iterator : public IteratorFacade<Iterator, T, std::random_access_iterator_tag> {
            friend class IteratorCore;
            T **cursor;
            
            public:
                Iterator(T **p) noexcept : cursor(p) {}
                Iterator(const Iterator &i) noexcept : cursor(i.cursor) {}
                
                T*   get() noexcept { return *cursor; }
                void set(T *p) { *cursor = p; }
            private:
                void increment() noexcept { ++cursor; }
                void decrement() noexcept { --cursor; }
                long compare(const Iterator &i) const noexcept { return cursor - i.cursor; }
                void advance(long diff) noexcept { cursor += diff; }
        };
        class ConstIterator : public IteratorFacade<ConstIterator, const T, std::random_access_iterator_tag> {
            friend class IteratorCore;
            const T * const * cursor;
            
            public:
                ConstIterator(const T * const *p) noexcept : cursor(p) {}
                ConstIterator(const ConstIterator &i) noexcept : cursor(i.cursor) {}
                
                const T * const get() noexcept { return *cursor; }
            private:
                void increment() noexcept { ++cursor; }
                void decrement() noexcept { --cursor; }
                long compare(const ConstIterator &i) const noexcept { return cursor - i.cursor; }
                void advance(long diff) noexcept { cursor += diff; }
        };
    private:
        T*& refAt(unsigned index) noexcept { return reinterpret_cast<T*&>(m_data[index]); }
        const T*const& refAt(unsigned index) const noexcept { return reinterpret_cast<const T*const&>(m_data[index]); }
    public:
        explicit PtrList(unsigned reserve = 16) : base(reserve) {}
        PtrList(const self &o) : base(o) {}
        PtrList(self &&o) noexcept : base(o) {}
        
        ~PtrList() noexcept = default;
        
        using base::empty;
        using base::clear;
        using base::size;
        using base::capacity;
        using base::reserve;
        using base::swap;
        using base::toHeap;
        
        void pushBack(T *data)  { base::pushBack(data); }
        T*    popBack() noexcept { return static_cast<T*>(base::popBack()); }
        
        T*       at(unsigned index)       noexcept { return static_cast<T*>(m_data[index]); }
        const T* at(unsigned index) const noexcept { return static_cast<const T*>(m_data[index]); }
        
        T*       operator[](unsigned index)       noexcept { return at(index); }
        const T* operator[](unsigned index) const noexcept { return at(index); }
        
        Iterator begin() noexcept { return Iterator{&refAt(0)}; }
        Iterator end()   noexcept { return Iterator{&refAt(m_len)}; }
        
        ConstIterator cbegin() const noexcept { return ConstIterator{&refAt(0)}; }
        ConstIterator cend()   const noexcept { return ConstIterator{&refAt(m_len)}; }
        
        ConstIterator begin() const noexcept { return cbegin(); }
        ConstIterator end()   const noexcept { return cend(); }
        
        self& operator=(self o) { this->swap(o); return *this; }
};

#define EXPORT(type, alias) \
    extern template class PtrList<type>; \
    using alias = PtrList<type>

EXPORT(Lexical::Token, TokenList);
EXPORT(Semantic::Stmt, StmtList);
EXPORT(Semantic::Decl, DeclList);
EXPORT(Semantic::Expr, ExprList);

#undef EXPORT

}
}

#endif // PTRLIST_HPP
