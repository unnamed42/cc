#ifndef TYPE_HPP
#define TYPE_HPP

#include "common.hpp"
#include "utils/ptrlist.hpp"
#include "semantic/qualtype.hpp"

namespace Compiler {

namespace Diagnostic {
class Logger;
}

namespace Semantic {

class VoidType;
class NumberType;
class PointerType;
class ArrayType;
class StructType;
class EnumType;
class FuncType;

class StructDecl;

class Type {
    protected:
        enum TypeTag {
            Void, Number, Pointer, Array, Struct, Enum, Func
        };
    private:
        TypeTag m_tag;
    protected:
        explicit Type(TypeTag tag) noexcept : m_tag(tag) {}
    public:
#define DECLARE_TYPECAST(type) \
    type##Type*       to##type()       noexcept; \
    const type##Type* to##type() const noexcept
        DECLARE_TYPECAST(Void);
        DECLARE_TYPECAST(Number);
        DECLARE_TYPECAST(Pointer);
        DECLARE_TYPECAST(Array);
        DECLARE_TYPECAST(Struct);
        DECLARE_TYPECAST(Enum);
        DECLARE_TYPECAST(Func);
#undef DECLARE_TYPECAST
        
        bool isScalar()    noexcept { return toNumber() || toPointer(); }
        bool isAggregate() noexcept { return toStruct() || toArray(); }
        
        virtual bool isComplete() const noexcept { return true; }
        
        virtual bool isCompatible(Type *that)    noexcept { return this == that; }
                bool isCompatible(QualType that) noexcept { return isCompatible(that.get()); }
        
        virtual unsigned size()  const noexcept { return 0; }
        virtual unsigned align() const noexcept { return size(); }
        
        virtual void print(Diagnostic::Logger&) const = 0;
        
        virtual Type* clone() { return this; }
};

/* C99 6.2.5 Types
 * 
 * The void type comprises an empty set of values; it is an incomplete type 
 * that cannot be completed.
 */
class VoidType : public Type {
    public:
        VoidType() noexcept : Type(TypeTag::Void) {}
        
        bool isComplete() const noexcept override { return false; }
        
        void print(Diagnostic::Logger&) const override;
};

/* C99 6.2.5 Types
 * 
 * There are five standard signed integer types, 
 * designated as `signed char`, `short int`, `int`, `long int`, and `long long int`.
 * The standard and extended signed integer types are collectively called `signed integer types`
 *
 * For each of the signed integer types, there is a corresponding (but different)
 * unsigned integer type (designated with the keyword unsigned).
 * The standard and extended unsigned integer types are collectively called `unsigned integer types`.
 *
 * The standard signed integer types and standard unsigned integer types are collectively
 * called the standard integer types, the extended signed integer types and extended
 * unsigned integer types are collectively called the extended integer types.
 *
 * For any two integer types with the same signedness and different integer conversion rank
 * (see 6.3.1.1), the range of values of the type with **smaller** integer conversion rank is a
 * subrange of the values of the other type.
 *
 * There are three real floating types, designated as float, double, and long double.
 * The set of values of the type float is a subset of the set of values of the
 * type double; the set of values of the type double is a subset of the set of values of the
 * type long double.
 *
 * The type char, the signed and unsigned integer types, and the floating types are
 * collectively called the basic types.
 *
 * The type char, the signed and unsigned integer types, and the enumerated types are
 * collectively called **integer types**. The integer and real floating types are collectively called
 * **real types**.
 *
 * Integer and floating types are collectively called **arithmetic types**.
 */
class NumberType : public Type {
    private:
        const uint32_t type;
    public:
        NumberType(uint32_t t) noexcept : Type(TypeTag::Number), type(t) {}
        
        void print(Diagnostic::Logger&) const override;
        
        unsigned size()  const noexcept override;
        unsigned align() const noexcept override;
        
        bool isSigned()   const noexcept;
        bool isUnsigned() const noexcept;
        
        bool isBool()       const noexcept;
        bool isChar()       const noexcept;
        bool isShort()      const noexcept;
        bool isInt()        const noexcept;
        bool isLong()       const noexcept;
        bool isLongLong()   const noexcept;
        bool isFloat()      const noexcept;
        bool isDouble()     const noexcept;
        bool isLongDouble() const noexcept;
        bool isIntegral()   const noexcept;
        bool isFraction()   const noexcept;
        
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
        unsigned rank() const noexcept;
        
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
        NumberType* promote() noexcept;
};

/* C99 6.2.5 Types
 * 
 * Any number of derived types can be constructed from the object, function, and
 * incomplete types, as follows:
 *
 * — An array type describes a contiguously allocated nonempty set of objects with a
 *   particular member object type, called the element type.
 * 
 *   Array types are characterized by their element type and by the 
 *   number of elements in the array. An array type is said to be derived from its element type, 
 *   and if its element type is T, the array type is sometimes called "array of T". 
 * 
 *   The construction of an array type from an element type is called "array type derivation".
 *
 * — A structure type describes a sequentially allocated nonempty set of member objects
 *   (and, in certain circumstances, an incomplete array), each of which has an optionally
 *   specified name and possibly distinct type.
 *
 * — A union type describes an overlapping nonempty set of member objects, each of
 *   which has an optionally specified name and possibly distinct type.
 *
 * — A function type describes a function with specified return type. A function type is
 *   characterized by its return type and the number and types of its parameters. 
 *   
 *   A function type is said to be derived from its return type, and if its return type is T, the
 *   function type is sometimes called "function returning T". The construction of a
 *   function type from a return type is called "function type derivation".
 *
 * — A pointer type may be derived from a function type, an object type, or an incomplete
 *   type, called the referenced type. A pointer type describes an object whose value
 *   provides a reference to an entity of the referenced type. 
 * 
 *   A pointer type derived from the referenced type T is sometimes called "pointer to T".
 *   The construction of a pointer type from a referenced type is called "pointer type derivation".
 *
 * These methods of constructing derived types can be applied recursively.
 *
 * Array, function, and pointer types are collectively called derived declarator types. A
 * declarator type derivation from a type T is the construction of a derived declarator type
 * from T by the application of an array-type, a function-type, or a pointer-type derivation to T.
 *
 * A type is characterized by its type category, which is either the outermost derivation of a
 * derived type (as noted above in the construction of derived types), or the type itself if the
 * type consists of no derived types.
 */
// class DerivedType : public Type {
//     private: 
//         QualType m_base;
//     protected:
//         explicit DerivedType(QualType base) noexcept;
//     public:
//         unsigned size()  const noexcept override;
//         unsigned align() const noexcept override;
//         
//         QualType base() const noexcept;
//         void setBase(QualType base) noexcept;
// };

class ArrayType : public Type {
    private:
        QualType m_base;
        int      m_bound;
    public:
        explicit ArrayType(QualType base, int bound = 0) noexcept
            : Type(TypeTag::Array), m_base(base), m_bound(bound) {}
        
        void print(Diagnostic::Logger&) const override;
        
        unsigned size() const noexcept override { return isComplete() ? m_base->size() * m_bound : 0; }
        
        bool isComplete() const noexcept override { return m_bound != -1; }
        
        bool isCompatible(Type*) noexcept override;
        
        ArrayType* clone() override;
        
        QualType base()           const noexcept { return m_base; }
        void     setBase(QualType base) noexcept { m_base = base; }
        
        int  bound()       const noexcept { return m_bound; }
        void setBound(int bound) noexcept { m_bound = bound; }
};

class PointerType : public Type {
    private:
        QualType m_base;
    public:
        explicit PointerType(QualType base) noexcept
            : Type(TypeTag::Pointer), m_base(base) {}
        
        explicit PointerType(Type *base, uint32_t qual = 0) noexcept
            : PointerType(QualType{base, qual}) {}
        
        bool isCompatible(Type*) noexcept override;
        
        void print(Diagnostic::Logger&) const override;
        
        unsigned size()  const noexcept override;
        unsigned align() const noexcept override { return size(); }
        
        bool isVoidPtr() const noexcept { return m_base->toVoid(); }
        
        QualType base()           const noexcept { return m_base; }
        void     setBase(QualType base) noexcept { m_base = base; }
};

class StructType : public Type {
    private:
        StructDecl *m_decl;
    public:
        explicit StructType(StructDecl *decl) noexcept
            : Type(TypeTag::Struct), m_decl(decl) {}
        
        bool isComplete()  const noexcept override;
        bool isCompatible(Type*) noexcept override;
        
        unsigned size()  const noexcept override;
        unsigned align() const noexcept override;
        
        void print(Diagnostic::Logger&) const override;
        
        /**
         * Return list of members, use after checking isComplete().
         * @return list of members
         */
        Utils::DeclList& members() noexcept;
        
        StructDecl*       decl()       noexcept { return m_decl; }
        const StructDecl* decl() const noexcept { return m_decl; }
};

class EnumType : public Type {
    private:
        bool m_complete;
    public:
        explicit EnumType(bool c = false) noexcept
            : Type(TypeTag::Enum), m_complete(c) {}
        
        bool isComplete() const noexcept override { return m_complete; }
        
        unsigned size()  const noexcept override;
        unsigned align() const noexcept override;
        
        void setComplete(bool c) noexcept { m_complete = c; }
        
        void print(Diagnostic::Logger&) const override;
};

class FuncType : public Type {
    private:
        QualType        m_ret;
        Utils::DeclList m_params;
        bool            m_vaarg;
    public:
        FuncType(QualType ret, Utils::DeclList &&params, bool vaarg = false) noexcept
            : Type(TypeTag::Func), m_ret(ret), m_params(move(params)), m_vaarg(vaarg) {}
        
        bool isCompatible(Type*) noexcept override;
        
        void print(Diagnostic::Logger&) const override;
        
        QualType returnType()          const noexcept { return m_ret; }
        void     setReturnType(QualType ret) noexcept { m_ret = ret; }
        
        bool isVaArgs() const noexcept { return m_vaarg; }
        void setVaArgs(bool va) noexcept { m_vaarg = va; }
        
        Utils::DeclList& params() noexcept { return m_params; }
        void setParams(Utils::DeclList &&params) noexcept { m_params = move(params); }
};

VoidType*    makeVoidType();
NumberType*  makeNumberType(uint32_t spec);
PointerType* makePointerType(QualType base);
PointerType* makePointerType(Type *base, uint32_t qual = 0);
ArrayType*   makeArrayType(QualType base, int bound = -1);
StructType*  makeStructType(Utils::DeclList *members = nullptr);
EnumType*    makeEnumType(bool = false);
FuncType*    makeFuncType(QualType ret, Utils::DeclList &&params, bool vaarg);

} // namespace Semantic
} // namespace Compiler

#endif // TYPE_HPP
