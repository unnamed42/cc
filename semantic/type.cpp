#include "utils/mempool.hpp"
#include "semantic/type.hpp"
#include "semantic/typeenum.hpp"
#include "diagnostic/logger.hpp"

#include <cassert>

namespace impl = Compiler::Semantic;

using namespace Compiler::Text;
using namespace Compiler::Utils;
using namespace Compiler::Semantic;
using namespace Compiler::Diagnostic;

static NumberType* greater(NumberType *lhs, NumberType *rhs) {
    auto max = (lhs->rank() < rhs->rank()) ? rhs : lhs;
    if(max->isFloat())
        return max;
    // rank is type_mask without sign
    auto spec = max->rank();
    if(lhs->isUnsigned() || rhs->isUnsigned())
        spec |= Unsigned;
    return makeNumberType(spec);
}

VoidType*          Type::toVoid()          noexcept { return nullptr; }
const VoidType*    Type::toVoid()    const noexcept { return nullptr; }
NumberType*        Type::toNumber()        noexcept { return nullptr; }
const NumberType*  Type::toNumber()  const noexcept { return nullptr; }
DerivedType*       Type::toDerived()       noexcept { return nullptr; }
const DerivedType* Type::toDerived() const noexcept { return nullptr; }
PointerType*       Type::toPointer()       noexcept { return nullptr; }
const PointerType* Type::toPointer() const noexcept { return nullptr; }
ArrayType*         Type::toArray()         noexcept { return nullptr; }
const ArrayType*   Type::toArray()   const noexcept { return nullptr; }
StructType*        Type::toStruct()        noexcept { return nullptr; }
const StructType*  Type::toStruct()  const noexcept { return nullptr; }
EnumType*          Type::toEnum()          noexcept { return nullptr; }
const EnumType*    Type::toEnum()    const noexcept { return nullptr; }
FuncType*          Type::toFunc()          noexcept { return nullptr; }
const FuncType*    Type::toFunc()    const noexcept { return nullptr; }
bool Type::isScalar()    noexcept { return toNumber() || toPointer(); }
bool Type::isAggregate() noexcept { return toStruct() || toArray(); }
bool Type::isComplete() const noexcept { return true; }
bool Type::isCompatible(Type *that)    noexcept { return this == that; }
bool Type::isCompatible(QualType that) noexcept { return isCompatible(that.get()); }
unsigned Type::size()  const noexcept { return 0; }
unsigned Type::align() const noexcept { return size(); }
Type* Type::clone() { return this; }

bool VoidType::isComplete() const noexcept { return false; }
VoidType*       VoidType::toVoid()       noexcept { return this; }
const VoidType* VoidType::toVoid() const noexcept { return this; }
void VoidType::print(Logger &log) const { log << "void"; }

NumberType::NumberType(uint32_t t) noexcept : type(t) {}
NumberType*       NumberType::toNumber()       noexcept { return this; }
const NumberType* NumberType::toNumber() const noexcept { return this; }
void NumberType::print(Logger &log) const { log << Logger::specifiers << type; }
unsigned NumberType::size() const noexcept { return sizeOf(type); }
unsigned NumberType::align() const noexcept { return size(); }
bool NumberType::isSigned()     const noexcept { return !isUnsigned(); }
bool NumberType::isUnsigned()   const noexcept { return type & Unsigned; }
bool NumberType::isBool()       const noexcept { return type == Bool; }
bool NumberType::isChar()       const noexcept { return type & Char; }
bool NumberType::isShort()      const noexcept { return type & Short; }
bool NumberType::isInt()        const noexcept { return type & Int; }
bool NumberType::isLong()       const noexcept { return type & Long; }
bool NumberType::isLongLong()   const noexcept { return type & LLong; }
bool NumberType::isFloat()      const noexcept { return type == Float; }
bool NumberType::isDouble()     const noexcept { return type == Double; }
bool NumberType::isLongDouble() const noexcept { return type == (Long|Double); }
bool NumberType::isIntegral()   const noexcept { return type & Integer; }
bool NumberType::isFraction()   const noexcept { return type & Floating; }
unsigned NumberType::rank() const noexcept { return type & ~Sign; }
NumberType* NumberType::promote() noexcept {
    auto that = makeNumberType(isUnsigned() ? Unsigned|Int : Int);
    return rank() <= that->rank() ? that : this;
}

DerivedType::DerivedType(QualType base) noexcept : m_base(base) {}
DerivedType*       DerivedType::toDerived()       noexcept { return this; }
const DerivedType* DerivedType::toDerived() const noexcept { return this; }
unsigned DerivedType::size()  const noexcept { return m_base->size(); }
unsigned DerivedType::align() const noexcept { return m_base->align(); }
QualType DerivedType::base()           const noexcept { return m_base; }
void     DerivedType::setBase(QualType base) noexcept { m_base = base; }

ArrayType::ArrayType(QualType base, int bound) noexcept : DerivedType(base), m_bound(bound) {}
ArrayType*       ArrayType::toArray()       noexcept { return this; }
const ArrayType* ArrayType::toArray() const noexcept { return this; }
unsigned ArrayType::size() const noexcept { return isComplete() ? base()->size() * m_bound : 0; }
bool ArrayType::isComplete() const noexcept { return m_bound != -1; }
ArrayType* ArrayType::clone() { return isComplete() ? this : makeArrayType(base(), m_bound); }
int  ArrayType::bound()       const noexcept { return m_bound; }
void ArrayType::setBound(int bound) noexcept { m_bound = bound; }
bool ArrayType::isCompatible(Type *that) noexcept {
    auto p = that->toArray();
    return p && p->m_bound == m_bound && base()->isCompatible(p->base());
}
void ArrayType::print(Logger &log) const {
    log << base() << '[';
    if(isComplete())
        log << m_bound;
    log << ']';
}

PointerType::PointerType(QualType base) noexcept : DerivedType(base) {}
PointerType::PointerType(Type *base, uint32_t qual) noexcept : PointerType(QualType{base, qual}) {}
PointerType*       PointerType::toPointer()       noexcept { return this; }
const PointerType* PointerType::toPointer() const noexcept { return this; }
void PointerType::print(Logger &log) const { log << base() << '*'; }
unsigned PointerType::size()  const noexcept { return SizePointer; }
unsigned PointerType::align() const noexcept { return size(); }
bool PointerType::isVoidPtr() const noexcept { return base()->toVoid(); }
bool PointerType::isCompatible(Type *that) noexcept {
    auto p = that->toPointer();
    return p && base()->isCompatible(p->base());
}

StructType::StructType(PtrList *list) noexcept : m_members(list) {}
StructType*       StructType::toStruct()       noexcept { return this; }
const StructType* StructType::toStruct() const noexcept { return this; }
bool StructType::isComplete() const noexcept { return m_members != nullptr; }
bool StructType::isCompatible(Type *other) noexcept {}
unsigned StructType::size()  const noexcept {}
unsigned StructType::align() const noexcept {}
PtrList& StructType::members() noexcept { return *m_members; }
void StructType::setMembers(PtrList *members) { m_members = members; }
void StructType::print(Logger &log) const {
    
}

FuncType::FuncType(QualType ret, PtrList &&list, bool vaarg) noexcept 
    : DerivedType(ret), m_params(static_cast<PtrList&&>(list)), m_vaarg(vaarg) {}
FuncType*       FuncType::toFunc()       noexcept { return this; }
const FuncType* FuncType::toFunc() const noexcept { return this; }
QualType FuncType::returnType()          const noexcept { return base(); }
void     FuncType::setReturnType(QualType ret) noexcept { setBase(ret); }
bool FuncType::isVaArgs()       const noexcept { return m_vaarg; }
void FuncType::setVaArgs(bool vaargs) noexcept { m_vaarg = vaargs; }
PtrList& FuncType::params()                    noexcept { return m_params; }
void     FuncType::setParams(PtrList &&params) noexcept { m_params = static_cast<PtrList&&>(params); }
void FuncType::print(Logger &log) const {
    
}

VoidType* impl::makeVoidType() {
    static VoidType *v = new (pool.align8Allocate(sizeof(VoidType))) VoidType{};
    return v;
}

NumberType* impl::makeNumberType(uint32_t spec) {
    #define PLACEMENT_NEW(typecode) new (pool.align8Allocate(sizeof(NumberType))) NumberType(typecode)
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
    
    #undef DECLARE_NUMBER_TYPE
    #undef PLACEMENT_NEW
    
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
}

PointerType* impl::makePointerType(QualType base) {
    return new (pool.align8Allocate(sizeof(PointerType))) PointerType(base);
}

PointerType* impl::makePointerType(Type *type, uint32_t qual) {
    return new (pool.align8Allocate(sizeof(PointerType))) PointerType(type, qual);
}

ArrayType* impl::makeArrayType(QualType base, int bound) {
    return new (pool.align8Allocate(sizeof(ArrayType))) ArrayType(base, bound);
}

StructType* impl::makeStructType(PtrList *members) {
    return new (pool.align8Allocate(sizeof(StructType))) StructType(members);
}

FuncType* impl::makeFuncType(QualType ret, PtrList &&list, bool vaarg) {
    return new (pool.align8Allocate(sizeof(FuncType))) FuncType(ret, static_cast<PtrList&&>(list), vaarg);
}