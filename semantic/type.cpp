#include "utils/mempool.hpp"
#include "text/ustring.hpp"
#include "semantic/type.hpp"
#include "semantic/typeenum.hpp"

#include <new>
#include <cassert>

namespace impl = Compiler::Semantic;

using namespace Compiler::Text;
using namespace Compiler::Utils;
using namespace Compiler::Semantic;

static TypeNumber* greater(TypeNumber *lhs, TypeNumber *rhs) {
    auto max = (lhs->rank() < rhs->rank()) ? rhs : lhs;
    if(max->isFloat())
        return max;
    // rank is type_mask without sign
    auto spec = max->rank();
    if(lhs->isUnsigned() || rhs->isUnsigned())
        spec |= Unsigned;
    return makeNumberType(spec);
}

TypeVoid*           Type::toVoid()           noexcept { return nullptr; }
const TypeVoid*     Type::toVoid()     const noexcept { return nullptr; }
TypeNumber*         Type::toNumber()         noexcept { return nullptr; }
const TypeNumber*   Type::toNumber()   const noexcept { return nullptr; }
TypeDerived*        Type::toDerived()        noexcept { return nullptr; }
const TypeDerived*  Type::toDerived()  const noexcept { return nullptr; }
TypePointer*        Type::toPointer()        noexcept { return nullptr; }
const TypePointer*  Type::toPointer()  const noexcept { return nullptr; }
TypeArray*          Type::toArray()          noexcept { return nullptr; }
const TypeArray*    Type::toArray()    const noexcept { return nullptr; }
TypeStruct*         Type::toStruct()         noexcept { return nullptr; }
const TypeStruct*   Type::toStruct()   const noexcept { return nullptr; }
TypeEnum*           Type::toEnum()           noexcept { return nullptr; }
const TypeEnum*     Type::toEnum()     const noexcept { return nullptr; }
TypeFunction*       Type::toFunction()       noexcept { return nullptr; }
const TypeFunction* Type::toFunction() const noexcept { return nullptr; }
bool Type::isScalar()    noexcept { return toNumber() || toPointer(); }
bool Type::isAggregate() noexcept { return toStruct() || toArray(); }
bool Type::isComplete() const noexcept { return true; }
bool Type::isCompatible(Type *that)    noexcept { return this == that; }
bool Type::isCompatible(QualType that) noexcept { return isCompatible(that.get()); }
unsigned Type::size()  const noexcept { return 0; }
unsigned Type::align() const noexcept { return size(); }
Type* Type::clone() { return this; }

bool TypeVoid::isComplete() const noexcept { return false; }
TypeVoid*       TypeVoid::toVoid()       noexcept { return this; }
const TypeVoid* TypeVoid::toVoid() const noexcept { return this; }
UString TypeVoid::toString() const { return "void"; }

TypeNumber::TypeNumber(uint32_t t) noexcept : type(t) {}
TypeNumber*       TypeNumber::toNumber()       noexcept { return this; }
const TypeNumber* TypeNumber::toNumber() const noexcept { return this; }
UString TypeNumber::toString() const { return specifierToString(type); }
unsigned TypeNumber::size() const noexcept { return sizeOf(type); }
unsigned TypeNumber::align() const noexcept { return size(); }
bool TypeNumber::isSigned()     const noexcept { return !isUnsigned(); }
bool TypeNumber::isUnsigned()   const noexcept { return type & Unsigned; }
bool TypeNumber::isBool()       const noexcept { return type == Bool; }
bool TypeNumber::isChar()       const noexcept { return type & Char; }
bool TypeNumber::isShort()      const noexcept { return type & Short; }
bool TypeNumber::isInt()        const noexcept { return type & Int; }
bool TypeNumber::isLong()       const noexcept { return type & Long; }
bool TypeNumber::isLongLong()   const noexcept { return type & LLong; }
bool TypeNumber::isIntegral()   const noexcept { return type & Integer; }
bool TypeNumber::isFloat()      const noexcept { return type == Float; }
bool TypeNumber::isDouble()     const noexcept { return type == Double; }
bool TypeNumber::isLongDouble() const noexcept { return type == (Long|Double); }
unsigned TypeNumber::rank() const noexcept { return type & ~Sign; }
TypeNumber* TypeNumber::promote() noexcept {
    auto that = makeNumberType(isUnsigned() ? Unsigned|Int : Int);
    return rank() <= that->rank() ? that : this;
}

TypeDerived::TypeDerived(QualType base) noexcept : base(base) {}
TypeDerived*       TypeDerived::toDerived()       noexcept { return this; }
const TypeDerived* TypeDerived::toDerived() const noexcept { return this; }
unsigned TypeDerived::size()  const noexcept { return base->size(); }
unsigned TypeDerived::align() const noexcept { return base->align(); }
QualType TypeDerived::get() const noexcept { return base; }
void TypeDerived::set(QualType base) noexcept { this->base = base; }

TypeArray::TypeArray(QualType base, int bound) noexcept : TypeDerived(base), m_bound(bound) {}
TypeArray*       TypeArray::toArray()       noexcept { return this; }
const TypeArray* TypeArray::toArray() const noexcept { return this; }
unsigned TypeArray::size() const noexcept { return isComplete() ? base->size() * m_bound : 0; }
bool TypeArray::isComplete() const noexcept { return m_bound != -1; }
TypeArray* TypeArray::clone() { return isComplete() ? this : makeArrayType(base, m_bound); }
int TypeArray::bound() const noexcept { return m_bound; }
void TypeArray::setBound(int bound) noexcept { m_bound = bound; }
bool TypeArray::isCompatible(Type *that) noexcept {
    auto p = that->toArray();
    return p && p->m_bound == m_bound && base->isCompatible(p->get());
}
UString TypeArray::toString() const {
    auto ret = base.toString();
    ret += '[';
    if(isComplete())
        ret += UString::fromUnsigned(m_bound);
    return ret += ']';
}

TypePointer::TypePointer(QualType base) noexcept : TypeDerived(base) {}
TypePointer::TypePointer(Type *base, uint32_t qual) noexcept : TypePointer(QualType{base, qual}) {}
TypePointer*       TypePointer::toPointer()       noexcept { return this; }
const TypePointer* TypePointer::toPointer() const noexcept { return this; }
UString TypePointer::toString() const { return base.toString() + '*'; }
unsigned TypePointer::size()  const noexcept { return SizePointer; }
unsigned TypePointer::align() const noexcept { return size(); }
bool TypePointer::isVoidPtr() const noexcept { return base->toVoid(); }
bool TypePointer::isCompatible(Type *that) noexcept {
    auto p = that->toPointer();
    return p && base->isCompatible(p->get());
}

TypeVoid* impl::makeVoidType() {
    static TypeVoid *v = new (pool.align8Allocate(sizeof(TypeVoid))) TypeVoid{};
    return v;
}

TypeNumber* impl::makeNumberType(uint32_t spec) {
    #define PLACEMENT_NEW(typecode) new (pool.align8Allocate(sizeof(TypeNumber))) TypeNumber(typecode)
    #define DECLARE_NUMBER_TYPE(name, typecode) static auto name = PLACEMENT_NEW(typecode)
    
    DECLARE_NUMBER_TYPE(boolt, Bool);
    DECLARE_NUMBER_TYPE(chart, Char);
    DECLARE_NUMBER_TYPE(schart, Signed|Char);
    DECLARE_NUMBER_TYPE(uchart, Unsigned|Char);
    DECLARE_NUMBER_TYPE(shortt, Short);
    DECLARE_NUMBER_TYPE(ushortt, Unsigned|Short);
    DECLARE_NUMBER_TYPE(intt, Int);
    DECLARE_NUMBER_TYPE(uintt, Unsigned|Int);
    DECLARE_NUMBER_TYPE(longt, Long);
    DECLARE_NUMBER_TYPE(ulongt, Unsigned|Long);
    DECLARE_NUMBER_TYPE(llongt, LLong);
    DECLARE_NUMBER_TYPE(ullongt, Unsigned|LLong);
    DECLARE_NUMBER_TYPE(floatt, Float);
    DECLARE_NUMBER_TYPE(doublet, Double);
    DECLARE_NUMBER_TYPE(ldoublet, Long|Double);
    
    switch(spec) {
        case Bool: return boolt;
        case Char: return chart;
        case Signed|Char: return schart;
        case Unsigned|Char: return uchart;
        case Short: case Signed|Short:
        case Short|Int: case Signed|Short|Int:
            return shortt;
        case Unsigned|Short: case Unsigned|Short|Int: 
            return ushortt;
        case Int: case Signed: case Signed|Int:
            return intt;
        case Unsigned: case Unsigned|Int: 
            return uintt;
        case Long: case Signed|Long:
        case Long|Int: case Signed|Long|Int:
            return longt;
        case Unsigned|Long: case Unsigned|Long|Int: 
            return ulongt;
        case LLong: case Signed|LLong:
        case LLong|Int: case Signed|LLong|Int:
            return llongt;
        case Unsigned|LLong: case Unsigned|LLong|Int:
            return ullongt;
        case Float: return floatt;
        case Double: return doublet;
        case Long|Double: return ldoublet;
        // case Float|Complex: case Double|Complex: case Long|Double|Complex:
        default: assert(false);
    }
    
    #undef DECLARE_NUMBER_TYPE
    #undef PLACEMENT_NEW
}

TypePointer* impl::makePointerType(Type *type, uint32_t qual) {
    return new (pool.align8Allocate(sizeof(TypePointer))) TypePointer(type, qual);
}

TypeArray* impl::makeArrayType(QualType base, int bound) {
    return new (pool.align8Allocate(sizeof(TypeArray))) TypeArray(base, bound);
}
