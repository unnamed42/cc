#include "ast.hpp"
#include "type.hpp"
#include "error.hpp"
#include "mempool.hpp"

using namespace compiler;

static mempool<type_array>   arr_pool{};
static mempool<type_pointer> ptr_pool{};
static mempool<type_struct>  struct_pool{};
static mempool<type_func>    func_pool{};
static mempool<type_enum>    enum_pool{};

const qual_type compiler::qual_null{};

// 32-bit machine
enum size_: unsigned int {
    size_bool = 1,
    size_char = 1,
    size_short = 2,
    size_int = 4,
    size_long = 4,
    size_llong = 8,
    size_float = 4,
    size_double = 8,
    size_ldouble = 8,
    
    size_ptr = 4,
};

static const char* spec_to_string(uint32_t mask) {
    switch(mask) {
        case Void: return "void";
        case Bool: return "bool";
        case Char: return "char";
        case Short: return "short";
        case Int: return "int";
        case Long: return "long";
        case LLong: return "long long";
        case Float: return "float";
        case Double: return "double";
        case Complex: return "complex";
        case Unsigned: return "unsigned";
        case Signed: return "signed";
        default: return "";
    }
}

static const char* qual_to_string(uint32_t mask) {
    switch(mask) {
        case Const: return "const";
        case Volatile: return "volatile";
        case Restrict: return "restrict";
        default: return "";
    }
}

static const char* storage_to_string(uint8_t stor) {
    switch(stor) {
        case Typedef: return "typedef";
        case Static: return "static";
        case Inline: return "inline";
        case Register: return "register";
        case Extern: return "extern";
        default: return "";
    }
}

static unsigned int index(uint32_t mask) {
    unsigned int i = 0;
    for(; mask; ++i) mask >>= 1;
    // The lowest mask code is 1
    return --i;
}

uint32_t compiler::attr_to_spec(uint32_t attr) {
    switch(attr) {
        case KeyVoid: return Void;
        case KeyBool: return Bool;
        case KeyChar: return Char;
        case KeyShort: return Short;
        case KeyInt: return Int;
        case KeyLong: return Long;
        case KeyFloat: return Float;
        case KeyDouble: return Double;
        case KeyComplex: return Complex;
        case KeyUnsigned: return Unsigned;
        case KeySigned: return Signed;
        case KeyTypedef: return Typedef;
        case KeyStatic: return Static;
        case KeyInline: return Inline;
        case KeyRegister: return Register;
        case KeyExtern: return Extern;
        case KeyStruct: 
        case KeyUnion: 
        case KeyEnum:
        case Identifier: return 0;
        case KeyConst: return Const;
        case KeyVolatile: return Volatile;
        case KeyRestrict: return Restrict;
        default: return -1;
    }
}

uint32_t compiler::calc_padding(uint32_t offset, uint32_t align) {
    // if align is power of 2, then
    // return (-offset) & (align - 1);
    return (align - (offset % align)) % align;
}

uint32_t compiler::padded_offset(uint32_t offset, uint32_t align) {
    // if align is power of 2
    // return (offset + align - 1) & ~(align - 1);
    return offset + calc_padding(offset, align);
}

/* C99 6.7.3 Type qualifiers
 * 
 * If the same qualifier appears more than once in the same specifier-qualifier-list, either
 * directly or via one or more typedefs, the behavior is the same as if it appeared only
 * once.
 */
uint8_t compiler::apply_qual(uint8_t lhs, uint32_t rhs) {
    if(lhs & rhs) 
        warning(*epos, "Duplicate qualifier \"%s\"", qual_to_string(rhs));
    return lhs |= rhs;
}

uint8_t compiler::apply_storage(uint8_t lhs, uint32_t rhs) {
    static const uint32_t comp[] = { // Compatibility mask
        0, // Typedef
        Inline, // Static
        Static, // Inline
        0, // Register
        0, // Extern
    };
    
    if(lhs & ~comp[index(rhs)]) 
        error(*epos, "Cannot apply storage class specifier \"%s\" to previous one", storage_to_string(rhs));
    else if(rhs & Register) 
        warning(*epos, "Deprecated storage class specifier \"register\", it has no effect");
    return lhs |= rhs;
}

uint32_t compiler::apply_spec(uint32_t lhs, uint32_t rhs) {
    static const uint32_t comp[12] = { // Compatibility masks
        0, // Void
        0, // Bool
        Signed | Unsigned, //Char
        Signed | Unsigned | Int, // Short
        Signed | Unsigned | Short | Long | LLong, // Int
        Signed | Unsigned | Long | Int, // Long
        Signed | Unsigned | Int, // LLong
        Complex, // Float
        Long | Complex, // Double
        Float | Double | Long, // Complex
        Char | Short | Int | Long | LLong, // Unsigned 
        Char | Short | Int | Long | LLong, // Signed
    };
    
    // incompatible two specifiers
    if(lhs & ~comp[index(rhs)])
        error(*epos, "Cannot apply specifier \"%s\" to previous combination", spec_to_string(rhs));
    
    if((lhs & Long) && (rhs & Long)) {
        lhs ^= Long;
        lhs |= LLong;
    } else 
        lhs |= rhs;
    
    return lhs;
}

qual_type qual_type::decay() const {
    auto ptr = const_cast<type*>(get());
    uint8_t qual = 0;
    if(ptr->is_func())
        ;
    else if(ptr->is_array()) {
        auto base = ptr->to_derived()->get();
        qual = base.qual();
        ptr = base.get();
    } else 
        return *this;
    return make_qual(make_pointer(ptr), qual);
}

qual_type qual_type::copy() const {
    return make_qual(get()->copy(), qual());
}

std::string qual_type::to_string() const {
    std::string res = get()->to_string();
    auto qual = this->qual();
    for(uint8_t bit = 1; bit <= Restrict; bit <<= 1) {
        auto temp = qual & bit;
        if(temp) {
            res += ' ';
            res += qual_to_string(temp);
        }
    }
    return res;
}

unsigned int type_arith::size() const {
    switch(m_type) {
        case Bool: return size_bool;
        case Char: case Signed|Char: case Unsigned|Char: 
            return size_char;
        case Short: case Signed|Short:
        case Short|Int: case Signed|Short|Int:
        case Unsigned|Short: case Unsigned|Short|Int: 
            return size_short;
        case Int: case Signed: case Signed|Int:
        case Unsigned: case Unsigned|Int: 
            return size_int;
        case Long: case Signed|Long:
        case Long|Int: case Signed|Long|Int:
        case Unsigned|Long: case Unsigned|Long|Int: 
            return size_long;
        case LLong: case Signed|LLong:
        case LLong|Int: case Signed|LLong|Int:
        case Unsigned|LLong: case Unsigned|LLong|Int:
            return size_llong;
        case Float: return size_float;
        case Double: return size_double;
        case Long|Double: return size_ldouble;
        default: error("Unknown type"); return 0;
    }
}

unsigned int type_arith::align() const {return size();}

std::string type_arith::to_string() const {
    std::string res{};
    
    auto sign = m_type & Sign;
    if(sign) {
        res += spec_to_string(sign);
        res += ' ';
    }
    
    for(uint32_t bit = 1; bit <= Double; bit <<= 1) {
        auto temp = m_type & bit;
        if(temp) {
            res += spec_to_string(temp);
            res += ' ';
        }
    }
    return res;
}

/* 
 * C99 6.3.1.1 Boolean, characters, and integers
 * 
 *  Every integer type has an integer conversion rank defined as follows:
 * 
 *  — No two signed integer types shall have the same rank, even if they have the same
 *    representation.
 *  — The rank of a signed integer type shall be greater than the rank of any signed integer
 *    type with less precision.
 *  — The rank of long long int shall be greater than the rank of long int, which
 *    shall be greater than the rank of int, which shall be greater than the rank of short
 *    int, which shall be greater than the rank of signed char.
 *  — The rank of any unsigned integer type shall equal the rank of the corresponding
 *    signed integer type, if any.
 *  — The rank of any standard integer type shall be greater than the rank of any extended
 *    integer type with the same width.
 *  — The rank of char shall equal the rank of signed char and unsigned char.
 *  — The rank of _Bool shall be less than the rank of all other standard integer types.
 *  — The rank of any enumerated type shall equal the rank of the compatible integer type
 *  — The rank of any extended signed integer type relative to another extended signed
 *    integer type with the same precision is implementation-defined, but still subject to the
 *    other rules for determining the integer conversion rank.
 *  — For all integer types T1, T2, and T3, if T1 has greater rank than T2 and T2 has
 *    greater rank than T3, then T1 has greater rank than T3.
 */
uint32_t type_arith::rank() const {
    return m_type & ~Sign;
}

/* C99 6.3.1.1 Boolean, characters, and integers
 *  
 * The following may be used in an expression wherever an int or unsigned int may
 * be used:
 * 
 * — An object or expression with an integer type whose integer conversion rank is less
 *   than or equal to the rank of int and unsigned int.
 * — A bit-field of type _Bool, int, signed int, or unsigned int.
 *  
 * If an int can represent all values of the original type, the value is converted to an int;
 * otherwise, it is converted to an unsigned int. These are called the integer
 * promotions.
 * 
 * All other types are unchanged by the integer promotions.
 * 
 * The integer promotions preserve value including sign. As discussed earlier, whether a
 * "plain" char is treated as signed is implementation-defined.
 */
type_arith* type_arith::promote() const {
    type_arith *int_type = make_arith(m_type & Unsigned ? Unsigned|Int : Int);
    
    if(rank() <= int_type->rank())
        return int_type;
    
    return const_cast<type_arith*>(this);
}

std::string type_array::to_string() const {
    auto res = m_base.to_string() + '[';
    if(m_len) res += std::to_string(m_len);
    return res += ']';    
}

type_array* type_array::copy() const {
    if(is_complete()) 
        return const_cast<type_array*>(this);
    return new (arr_pool.malloc()) type_array(m_base, m_len);
}

unsigned int type_pointer::size() const {return size_ptr;}

unsigned int type_pointer::align() const {return size_ptr;}

unsigned int type_struct::size() const {
    unsigned int res = 0;
    for(auto &&item: m_members)
        res += item->m_type->size();
    return res;
}

unsigned int type_enum::size() const {return size_int;}

unsigned int type_enum::align() const {return size();}

unsigned int type_func::size() const {return size_ptr;}

std::string type_func::to_string() const {
    std::string res{m_base.to_string() + '('};
    if(!unspec) 
        return res += ')';
    bool nfirst = false;
    for(auto &&param: m_params) {
        if(nfirst) res += ',';
        res += param->m_type.to_string();
        nfirst = true;
    }
    if(variadic) {
        if(nfirst) res += ',';
        res += "...";
    }
    return res += ')';
}

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
bool type_struct::compatible(const type &t) const {
    auto ptr = t.to_struct();
    if(this == ptr) return true;
    if(!ptr) return false;
    
    if(!is_complete() && !t.is_complete()) return this == &t;
    if(is_complete() != t.is_complete()) return false;
    
    auto &&t_member = ptr->m_members;
    
    if(t_member.size() != m_members.size())
        return false;
    
    auto it = t_member.cbegin();
    for(auto &&member: m_members) {
        // different name is allowed?
        if(!(*it)->m_type->compatible(member->m_type))
            return false;
    }
    return true;
}

bool type_func::compatible(const type &t) const {
    auto rhs = t.to_func();
    if(this == rhs) return true;
    if(!rhs) return false;
    
    if(!m_base->compatible(rhs->m_base) || variadic != rhs->variadic)
        return false;
    if(unspec) // an unspecified parameter list matches any list
        return true;
    auto lit = m_params.cbegin();
    auto &&rp = rhs->m_params; // right hand parameter list
    auto rit = rp.cbegin();
    if(m_params.size() != rp.size()) 
        return false;
    for(; lit != m_params.cend(); ++lit) {
        if(!(*lit)->m_type->compatible((*rit)->m_type))
            return false;
        ++rit;
    }
    return true;
}

qual_type compiler::make_qual(type *base, uint8_t qual) {
    return qual_type(base, qual);
}

qual_type compiler::make_void() {
    static type_void void_type;
    return make_qual(&void_type);
}

qual_type compiler::qual_arith(uint32_t base, uint8_t qual) {
    return make_qual(make_arith(base), qual);
}

type_arith* compiler::make_arith(uint32_t tp) {
    #define GENERATE_ARITH(name, qual) \
        static type_arith name(qual)
    GENERATE_ARITH(bool_type, Bool);
    GENERATE_ARITH(char_type, Char);
    GENERATE_ARITH(schar_type, Signed|Char);
    GENERATE_ARITH(uchar_type, Unsigned|Char);
    GENERATE_ARITH(short_type, Short);
    GENERATE_ARITH(ushort_type, Unsigned|Short);
    GENERATE_ARITH(int_type, Int);
    GENERATE_ARITH(uint_type, Unsigned|Int);
    GENERATE_ARITH(long_type, Long);
    GENERATE_ARITH(ulong_type, Unsigned|Long);
    GENERATE_ARITH(llong_type, LLong);
    GENERATE_ARITH(ullong_type, Unsigned|LLong);
    GENERATE_ARITH(float_type, Float);
    GENERATE_ARITH(double_type, Double);
    GENERATE_ARITH(ldouble_type, Long|Double);
    #undef GENERATE_ARITH
    
    switch(tp) {
        case Bool: return &bool_type;
        case Char: return &char_type;
        case Signed|Char: return &schar_type;
        case Unsigned|Char: return &uchar_type;
        case Short: case Signed|Short:
        case Short|Int: case Signed|Short|Int:
            return &short_type;
        case Unsigned|Short: case Unsigned|Short|Int: 
            return &ushort_type;
        case Int: case Signed: case Signed|Int:
            return &int_type;
        case Unsigned: case Unsigned|Int: 
            return &uint_type;
        case Long: case Signed|Long:
        case Long|Int: case Signed|Long|Int:
            return &long_type;
        case Unsigned|Long: case Unsigned|Long|Int: 
            return &ulong_type;
        case LLong: case Signed|LLong:
        case LLong|Int: case Signed|LLong|Int:
            return &llong_type;
        case Unsigned|LLong: case Unsigned|LLong|Int:
            return &ullong_type;
        case Float: return &float_type;
        case Double: return &double_type;
        case Long|Double: return &ldouble_type;
        // case Float|Complex: case Double|Complex: case Long|Double|Complex:
        default: error(*epos, "Invalid type-specifier combination for arithmetic type"); return nullptr;
    }
}

qual_type compiler::max_type(type_arith *lhs, type_arith *rhs) {
    auto max = (lhs->rank() < rhs->rank()) ? rhs : lhs;
    if(max->is_float())
        return make_qual(max);
    // rank is type_mask without sign
    auto spec = max->rank();
    if(lhs->is_unsigned() || lhs->is_unsigned())
        spec |= Unsigned;
    return qual_arith(spec);
}

qual_type compiler::make_array(qual_type base, unsigned int len) {
    auto arr = new (arr_pool.malloc()) type_array(base, len);
    return make_qual(arr);
}

type_pointer* compiler::make_pointer(type *base, uint8_t base_qual) {
    return new (ptr_pool.malloc()) type_pointer(base, base_qual);
}

qual_type compiler::qual_pointer(qual_type base, uint8_t qual) {
    auto ptr = make_pointer(base.get(), base.qual());
    return make_qual(ptr, qual);
}

type_struct* compiler::make_struct(scope *s) {
    return new (struct_pool.malloc()) type_struct(s);
}

type_struct* compiler::make_struct(scope *s, member_list &&m) {
    return new (struct_pool.malloc()) type_struct(s, std::move(m));
}

type_enum* compiler::make_enum() {
    return new (enum_pool.malloc()) type_enum();
}

qual_type compiler::make_func(qual_type ret, param_list &&par, bool va, bool unspecified) {
    auto func = new (func_pool.malloc()) type_func(ret, std::move(par), va, unspecified);
    return make_qual(func);
}
