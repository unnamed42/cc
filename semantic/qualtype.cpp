#include "text/ustring.hpp"
#include "semantic/type.hpp"
#include "semantic/typeenum.hpp"
#include "semantic/qualtype.hpp"

#include <cassert>

namespace impl = Compiler::Semantic;

using namespace Compiler;
using namespace Compiler::Text;
using namespace Compiler::Semantic;

const QualType impl::QualNull{nullptr, 0};

QualType::QualType(Type *type, uint32_t qual) noexcept {
    reset(type, qual);
}

QualType::QualType(const self &other) noexcept : m_ptr(other.m_ptr) {}

uint32_t QualType::qual() const noexcept {
    return m_ptr & Qual;
}

Type* QualType::get() noexcept {
    return reinterpret_cast<Type*>(m_ptr & ~Qual);
}

const Type* QualType::get() const noexcept {
    return reinterpret_cast<const Type*>(m_ptr & ~Qual);
}

void QualType::setQual(uint32_t qual) noexcept {
    m_ptr &= ~Qual;
    m_ptr |= qual;
}

void QualType::addQual(uint32_t qual) noexcept {
    m_ptr |= qual;
}

void QualType::setBase(Type *base) noexcept {
    m_ptr = reinterpret_cast<uintptr_t>(base) | qual();
}

Type* QualType::operator->() noexcept {
    return get();
}

const Type* QualType::operator->() const noexcept {
    return get();
}

void QualType::reset(Type *type, uint32_t qual) noexcept {
    m_ptr = reinterpret_cast<uintptr_t>(type) | qual;
    assert((reinterpret_cast<uintptr_t>(type) & qual) == 0);
}

bool QualType::isNull() const noexcept {
    return get() == nullptr;
}

bool QualType::isConst() const noexcept {
    return m_ptr & Const;
}

bool QualType::isVolatile() const noexcept {
    return m_ptr & Volatile;
}

bool QualType::isRestrict() const noexcept {
    return m_ptr & Restrict;
}

QualType QualType::decay() noexcept {
    auto ptr = get();
    uint32_t qual = 0;
    
    auto func = ptr->toFunction();
    auto array = ptr->toArray();
    
    if(func)
        ;
    else if(array) {
        auto base = array->get();
        qual = base.qual();
        ptr = base.get();
    } else 
        return *this;
    return {makePointerType(ptr), qual};
}

Text::UString QualType::toString() const {
    auto str{get()->toString()};
    str += ' ';
    return str += ::toString(static_cast<Qualifier>(qual()));
}

QualType::operator bool() const noexcept {
    return get() != nullptr;
}

bool QualType::operator==(const QualType &o) const noexcept {
    return m_ptr == o.m_ptr;
}

bool QualType::operator!=(const QualType &o) const noexcept {
    return !operator==(o);
}
