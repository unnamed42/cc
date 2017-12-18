#include "utils/ptrlist.hpp"
#include "utils/mempool.hpp"

#include <cstring>

using namespace Compiler;
using namespace Compiler::Utils;

template <class T>
static inline void swap(T &a, T &b) noexcept {
    T temp = a;
    a = b;
    b = temp;
}

#define INSTANTIATE(type) \
    template class Compiler::Utils::PtrList<type>
INSTANTIATE(Lexical::Token);
INSTANTIATE(Semantic::Stmt);
INSTANTIATE(Semantic::Decl);
INSTANTIATE(Semantic::Expr);

PtrListBase::PtrListBase(unsigned reserve) 
    : m_data(new (pool) void*[reserve]), m_len(0), m_cap(reserve) {}

PtrListBase::PtrListBase(const self &o) : PtrListBase(o.size()) {
    m_len = o.m_len;
    memcpy(m_data, o.m_data, sizeof(void*) * m_len);
}

PtrListBase::PtrListBase(self &&o) noexcept
    : m_data(o.m_data), m_len(o.m_len), m_cap(o.m_cap) { o.m_data = nullptr; }

PtrListBase::~PtrListBase() noexcept { operator delete[](m_data, sizeof(void*) * m_cap, pool); }

void PtrListBase::pushBack(void *data) {
    if(m_len == m_cap)
        reserve(m_cap + m_cap/2);
    m_data[m_len++] = data;
}

void* PtrListBase::popBack() noexcept {
    return m_data[--m_len];
}

void PtrListBase::append(const self &o) {
    auto osize = o.size();
    reserve(size() + osize);
    memcpy(m_data + m_len, o.m_data, osize * sizeof(void*));
    m_len += osize;
}

void PtrListBase::reserve(unsigned size) {
    if(size > m_cap) {
        auto p = pool.reallocate(m_data, m_cap * sizeof(void*), size * sizeof(void*));
        m_cap = size;
        m_data = static_cast<decltype(m_data)>(p);
    }
}

void PtrListBase::swap(self &o) noexcept {
    ::swap(m_data, o.m_data);
    ::swap(m_len, o.m_len);
    ::swap(m_cap, o.m_cap);
}
