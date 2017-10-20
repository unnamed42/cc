#ifndef VECTOR_H
#define VECTOR_H

#include "common.hpp"
#include "utils/mempool.hpp"
#include "utils/iterator.hpp"

#include <cstring>
#include <initializer_list>

namespace Compiler {

namespace Lexical {
class Token;
}
namespace Semantic {
class Decl;
class Expr;
class Stmt;
}

namespace Utils {

template <class T>
class Vector {
    public:
        using ValueType     = T;
        using self          = Vector<T>;
        using Iterator      = Utils::Iterator<T*>;
        using ConstIterator = Utils::ConstIterator<Iterator>;
    protected:
        ValueType *m_data;
        unsigned   m_len;
        unsigned   m_cap;
    public:
        Vector() : m_data(new (pool) ValueType[16]), m_len(0), m_cap(16) {}
        Vector(unsigned reserve) : Vector() { this->reserve(reserve); }
        Vector(std::initializer_list<T> list) { 
            m_len = m_cap = list.size();
            m_data = new (pool) ValueType[list.size()];
            memcpy(m_data, list.begin(), sizeof(ValueType) * m_cap); 
        }
        Vector(ValueType fill, unsigned count = 1) : Vector(count) {
            m_len = count;
            while(--count)
                m_data[count] = fill;
        }
        Vector(const self &other) : Vector(other.size()) {
            m_len = other.m_len;
            memcpy(m_data, other.m_data, sizeof(ValueType) * m_len);
        }
        Vector(self &&other) : m_data(other.m_data), m_len(other.m_len), m_cap(other.m_cap) {
            other.m_data = nullptr;
        }
        
        ~Vector() { operator delete[](m_data, sizeof(ValueType) * m_cap, pool); }
        
        /**
         * Move stack allocated Vector object into heap memory.
         * @return memory where Vector moved into
         */
        self* toHeap() {
            return new (pool) self(move(*this));
        }
        
        void swap(self &other) noexcept {
            auto p = m_data; m_data = other.m_data; other.m_data = p;
            unsigned t = m_len; m_len = other.m_len; other.m_len = t;
            t = m_cap; m_cap = other.m_cap; other.m_cap = t;
        }
        
        void clear() noexcept { m_len = 0; }
        void reserve(unsigned size) { 
            if(m_cap < size)
                resize(size);
        }
        
        unsigned size() const noexcept { return m_len; }
        unsigned capacity() const noexcept { return m_cap; }
        
        bool empty() const noexcept { return m_len == 0; }
        
        void pushBack(ValueType val) { 
            if(m_len == m_cap)
                resize(m_cap + m_cap/2);
            m_data[m_len++] = val;
        }
        T popBack() noexcept { return static_cast<T&&>(m_data[--m_len]); }
        
        Iterator begin() noexcept { return Iterator{m_data}; }
        Iterator end() noexcept { return Iterator{m_data + m_len}; }
        
        ConstIterator begin() const noexcept { return ConstIterator{m_data}; }
        ConstIterator end()   const noexcept { return ConstIterator{m_data + m_len}; }
        
        ValueType& front() noexcept { return m_data[0]; }
        ValueType  front() const noexcept { return m_data[0]; }
        ValueType& back() noexcept { return m_data[m_len - 1]; }
        ValueType  back() const noexcept { return m_data[m_len - 1]; }
        
        ValueType& at(unsigned index) noexcept { return m_data[index]; }
        ValueType  at(unsigned index) const noexcept { return m_data[index]; }
        
        ValueType& operator[](unsigned index) noexcept { return at(index); }
        ValueType  operator[](unsigned index) const noexcept { return at(index); }
        
        self& operator=(self other) { swap(other); return *this; }
    protected:
        void resize(unsigned size) {
            auto p = pool.reallocate(m_data, m_cap, size * sizeof(ValueType));
            m_cap = size;
            m_data = static_cast<decltype(m_data)>(p);
        }
};

extern template class Vector<Lexical::Token*>;
extern template class Vector<Semantic::Decl*>;
extern template class Vector<Semantic::Expr*>;
extern template class Vector<Semantic::Stmt*>;

using TokenList = Vector<Lexical::Token*>;
using DeclList = Vector<Semantic::Decl*>;
using ExprList = Vector<Semantic::Expr*>;
using StmtList = Vector<Semantic::Stmt*>;

} // namespace Utils
} // namespace Compiler

#endif // VECTOR_H
