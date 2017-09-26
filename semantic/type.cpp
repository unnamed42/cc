#include "utils/mempool.hpp"
#include "semantic/type.hpp"
#include "semantic/decl.hpp"
#include "semantic/typeenum.hpp"
#include "diagnostic/logger.hpp"

#include <cassert>

namespace impl = Compiler::Semantic;

using namespace Compiler::Text;
using namespace Compiler::Utils;
using namespace Compiler::Semantic;
using namespace Compiler::Diagnostic;

#define MAKE_TYPE(typename, ...) new (pool, MemPool::aligned) typename(__VA_ARGS__)

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

StructType::StructType(DeclList *list) noexcept : m_members(list) {}
StructType*       StructType::toStruct()       noexcept { return this; }
const StructType* StructType::toStruct() const noexcept { return this; }
bool StructType::isComplete() const noexcept { return m_members != nullptr; }
/* C99 6.2.7 Compatible type and composite type
 * 
 * Moreover, two structure, union, or enumerated types declared in separate translation units 
 * are compatible if their tags and members satisfy the following requirements: 
 * 
 *   If one is declared with a tag, the other shall be declared with the same tag. 
 * 
 *   If both are complete types, then the following additional requirements apply:
 * 
 *     there shall be a one-to-one correspondence between their members such that 
 *     each pair of corresponding members are declared with compatible types, and such that 
 *     if one member of a corresponding pair is declared with a name, the other member 
 *     is declared with the same name. For two structures, corresponding members 
 *     shall be declared in the same order. For two structures or unions, corresponding
 *     bit-fields shall have the same widths. For two enumerations, corresponding members
 *     shall have the same values.
 */
// FIXME: the rule gets looser by not requiring the same tag name
bool StructType::isCompatible(Type *other) noexcept {
    auto ptr = other->toStruct();
    if(!ptr)
        return false;
    if(!isComplete() && !ptr->isComplete())
        return this == ptr;
    if(isComplete() != ptr->isComplete())
        return false;
    
    auto &&otherMembers = ptr->members();
    auto otherMember = otherMembers.begin();
    auto otherEnd = otherMembers.end();
    for(auto &&member : *m_members) {
        if(otherMember == otherEnd)
            return false;
        // different name is allowed?
        if(member->type()->isCompatible((*otherMember)->type()))
            return false;
    }
    return true;
}
unsigned StructType::size()  const noexcept {
    unsigned ret = 0;
    if(!isComplete()) 
        return ret;
    for(auto member : *m_members)
        ret += member->type()->size();
    return ret;
}
unsigned StructType::align() const noexcept { return size(); }
DeclList& StructType::members() noexcept { return *m_members; }
void StructType::setMembers(DeclList *members) { m_members = members; }
void StructType::print(Logger &log) const {
    log << "struct";
    if(!m_members)
        return;
    log << '{';
    for(auto member : *m_members) 
        log << member->type() << ' ' << member->token() << ';';
    log << '}';
}

FuncType::FuncType(QualType ret, DeclList &&list, bool vaarg) noexcept 
    : DerivedType(ret), m_params(move(list)), m_vaarg(vaarg) {}
FuncType*       FuncType::toFunc()       noexcept { return this; }
const FuncType* FuncType::toFunc() const noexcept { return this; }
QualType FuncType::returnType()          const noexcept { return base(); }
void     FuncType::setReturnType(QualType ret) noexcept { setBase(ret); }
bool FuncType::isVaArgs()       const noexcept { return m_vaarg; }
void FuncType::setVaArgs(bool vaargs) noexcept { m_vaarg = vaargs; }
DeclList& FuncType::params()                    noexcept { return m_params; }
void     FuncType::setParams(DeclList &&params) noexcept { m_params = move(params); }
bool FuncType::isCompatible(Type *other) noexcept {
    if(this == other)
        return true;
    auto otherFunc = other->toFunc();
    if(!otherFunc) 
        return false;
    
    if(!returnType()->isCompatible(otherFunc->returnType()) || isVaArgs() != otherFunc->isVaArgs())
        return false;
    auto &&params = this->params();
    auto &&otherParams = otherFunc->params();
    if(params.size() == 0) // an unspecified parameter list matches any list
        return true;
    if(params.size() != otherParams.size())
        return false;
    
    auto otherParam = otherParams.begin();
    for(auto &&param : params) {
        if(!param->type()->isCompatible((*otherParam)->type()))
            return false;
    }
    return true;
}
void FuncType::print(Logger &log) const {
    log << returnType() << '(';
    bool first = true;
    for(auto param : m_params) {
        if(!first) {
            log << ", ";
            first = false;
        }
        log << param->type();
    }
    if(isVaArgs()) {
        if(!first)
            log << ", ";
        log << "...";
    }
    log << ')';
}

VoidType* impl::makeVoidType() {
    static VoidType *v = MAKE_TYPE(VoidType);
    return v;
}

NumberType* impl::makeNumberType(uint32_t spec) {
    #define DECLARE_NUMBER_TYPE(name, typecode) static auto name = MAKE_TYPE(NumberType, typecode)
    
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
    return MAKE_TYPE(PointerType, base);
}

PointerType* impl::makePointerType(Type *type, uint32_t qual) {
    return MAKE_TYPE(PointerType, type, qual);
}

ArrayType* impl::makeArrayType(QualType base, int bound) {
    return MAKE_TYPE(ArrayType, base, bound);
}

StructType* impl::makeStructType(DeclList *members) {
    return MAKE_TYPE(StructType, members);
}

FuncType* impl::makeFuncType(QualType ret, DeclList &&list, bool vaarg) {
    return MAKE_TYPE(FuncType, ret, move(list), vaarg);
}
