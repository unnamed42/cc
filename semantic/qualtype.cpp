#include "semantic/type.hpp"
#include "semantic/typeenum.hpp"
#include "semantic/qualtype.hpp"
#include "diagnostic/logger.hpp"

#include <cassert>

using namespace Compiler::Semantic;
using namespace Compiler::Diagnostic;

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

void QualType::reset(Type *type, uint32_t qual) noexcept {
    m_ptr = reinterpret_cast<uintptr_t>(type) | qual;
    assert((reinterpret_cast<uintptr_t>(type) & qual) == 0);
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
    
    auto func = ptr->toFunc();
    auto array = ptr->toArray();
    
    if(array) {
        auto base = array->base();
        qual = base.qual();
        ptr = base.get();
    } else if(!func)
        return *this;
    return QualType{ makePointerType(ptr), qual };
}

void QualType::print(Logger &log) noexcept {
    auto qual = this->qual();
    if(qual)
        log << Logger::qualifiers << qual << ' ';
    get()->print(log);
}
