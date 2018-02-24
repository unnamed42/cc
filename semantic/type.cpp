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

#define DEFINE_TYPECAST(type) \
    type##Type* Type::to##type() noexcept { return m_tag == type ? static_cast<type##Type*>(this) : nullptr; } \
    const type##Type* Type::to##type() const noexcept { return m_tag == type ? static_cast<const type##Type*>(this) : nullptr; }
DEFINE_TYPECAST(Void);
DEFINE_TYPECAST(Number);
DEFINE_TYPECAST(Pointer);
DEFINE_TYPECAST(Array);
DEFINE_TYPECAST(Struct);
DEFINE_TYPECAST(Enum);
DEFINE_TYPECAST(Func);

void VoidType::print(Logger &log) const { log << "void"; }

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

ArrayType* ArrayType::clone() { return isComplete() ? this : makeArrayType(m_base, m_bound); }
bool ArrayType::isCompatible(Type *that) noexcept {
    auto p = that->toArray();
    return p && p->m_bound == m_bound && m_base->isCompatible(p->m_base);
}
void ArrayType::print(Logger &log) const {
    log << m_base << '[';
    if(isComplete())
        log << m_bound;
    log << ']';
}

void PointerType::print(Logger &log) const { log << m_base << '*'; }
unsigned PointerType::size()  const noexcept { return SizePointer; }
bool PointerType::isCompatible(Type *that) noexcept {
    auto p = that->toPointer();
    return p && m_base->isCompatible(p->m_base);
}

unsigned EnumType::size()  const noexcept { return SizeInt; }
unsigned EnumType::align() const noexcept { return size(); }
void EnumType::print(Logger &log) const { log << "enum"; }

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
        if(member.type()->isCompatible(otherMember->type()))
            return false;
    }
    return true;
}
unsigned StructType::size()  const noexcept {
    unsigned ret = 0;
    if(!isComplete()) 
        return ret;
    for(auto member : *m_members)
        ret += member.type()->size();
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
        log << member.type() << ' ' << member.token() << ';';
    log << '}';
}

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
        if(!param.type()->isCompatible(otherParam->type()))
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
        log << param.type();
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

EnumType* impl::makeEnumType(bool complete) {
    return MAKE_TYPE(EnumType, complete);
}

StructType* impl::makeStructType(DeclList *members) {
    return MAKE_TYPE(StructType, members);
}

FuncType* impl::makeFuncType(QualType ret, DeclList &&list, bool vaarg) {
    return MAKE_TYPE(FuncType, ret, move(list), vaarg);
}
