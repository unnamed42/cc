#ifndef VECTOR_H
#define VECTOR_H

#include "utils/mempool.hpp"
#include "utils/iterator.hpp"

#include <cstring>

namespace Compiler {
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
        Vector() : m_data((ValueType*)pool.allocate(sizeof(ValueType)*16)), m_len(0), m_cap(16) {}
        Vector(unsigned reserve) : Vector() { this->reserve(reserve); }
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
        
        ~Vector() { pool.deallocate(m_data); }
        
        /**
         * Move stack allocated Vector object into heap memory.
         * @return memory where Vector moved into
         */
        self* toHeap() {
            return new (pool.allocate<self>()) self(std::move(*this));
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

} // namespace Utils
} // namespace Compiler

#endif // VECTOR_H
