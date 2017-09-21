#ifndef TYPE_HPP
#define TYPE_HPP

#include "utils/ptrlist.hpp"
#include "semantic/qualtype.hpp"

#include <cstdint>

namespace Compiler {

namespace Text {
class UString;
}

namespace Semantic {

class VoidType;
class NumberType;
class DerivedType;
class PointerType;
class ArrayType;
class StructType;
class EnumType;
class FuncType;

class Type {
    public:
        Type() noexcept = default;
        virtual ~Type() = default;
        
        // virtual functions to avoid dynamic_cast
        virtual VoidType*          toVoid()          noexcept;
        virtual const VoidType*    toVoid()    const noexcept;
        virtual NumberType*        toNumber()        noexcept;
        virtual const NumberType*  toNumber()  const noexcept;
        virtual DerivedType*       toDerived()       noexcept;
        virtual const DerivedType* toDerived() const noexcept;
        virtual PointerType*       toPointer()       noexcept;
        virtual const PointerType* toPointer() const noexcept;
        virtual ArrayType*         toArray()         noexcept;
        virtual const ArrayType*   toArray()   const noexcept;
        virtual StructType*        toStruct()        noexcept;
        virtual const StructType*  toStruct()  const noexcept;
        virtual EnumType*          toEnum()          noexcept;
        virtual const EnumType*    toEnum()    const noexcept;
        virtual FuncType*          toFunc()          noexcept;
        virtual const FuncType*    toFunc()    const noexcept;
        
        bool isScalar()    noexcept;
        bool isAggregate() noexcept;
        
        virtual bool isComplete() const noexcept;
        
        virtual bool isCompatible(Type*) noexcept;
        bool isCompatible(QualType) noexcept;
        
        virtual unsigned size()  const noexcept;
        virtual unsigned align() const noexcept;
        
        virtual Text::UString toString() const = 0;
        
        virtual Type* clone();
};

/* C99 6.2.5 Types
 * 
 * The void type comprises an empty set of values; it is an incomplete type 
 * that cannot be completed.
 */
class VoidType : public Type {
    public:
        VoidType() = default;
        
        bool isComplete() const noexcept override;
        
        VoidType*       toVoid()       noexcept override;
        const VoidType* toVoid() const noexcept override;
        
        Text::UString toString() const override;
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
        NumberType(uint32_t) noexcept;
        
        NumberType*       toNumber()       noexcept override;
        const NumberType* toNumber() const noexcept override;
        
        Text::UString toString() const override;
        
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
class DerivedType : public Type {
    private: 
        QualType m_base;
    protected:
        explicit DerivedType(QualType base) noexcept;
    public:
        DerivedType*       toDerived()       noexcept override;
        const DerivedType* toDerived() const noexcept override;
        
        unsigned size()  const noexcept override;
        unsigned align() const noexcept override;
        
        QualType base() const noexcept;
        void setBase(QualType base) noexcept;
};

class ArrayType : public DerivedType {
    private:
        int m_bound;
    public:
        explicit ArrayType(QualType base, int bound = 0) noexcept;
        
        ArrayType*       toArray()       noexcept override;
        const ArrayType* toArray() const noexcept override;
        
        Text::UString toString() const override;
        
        unsigned size() const noexcept override;
        
        bool isComplete() const noexcept override;
        
        bool isCompatible(Type*) noexcept override;
        
        ArrayType* clone() override;
        
        int bound() const noexcept;
        void setBound(int bound) noexcept;
};

class PointerType : public DerivedType {
    public:
        explicit PointerType(QualType base) noexcept;
        explicit PointerType(Type *base, uint32_t qual = 0) noexcept;
        
        bool isCompatible(Type*) noexcept override;
        
        PointerType*       toPointer()       noexcept override;
        const PointerType* toPointer() const noexcept override;
        
        Text::UString toString() const override;
        
        unsigned size()  const noexcept override;
        unsigned align() const noexcept override;
        
        bool isVoidPtr() const noexcept;
};

class StructType : public Type {
    private:
        /** list of Decl* */
        Utils::PtrList *m_members;
    public:
        StructType(Utils::PtrList * = nullptr) noexcept;
        
        StructType*       toStruct()       noexcept override;
        const StructType* toStruct() const noexcept override;
        
        bool isComplete() const noexcept override;
        bool isCompatible(Type*) noexcept override;
        
        unsigned size()  const noexcept override;
        unsigned align() const noexcept override;
        
        Text::UString toString() const override;
        
        Utils::PtrList& members() noexcept;
        void setMembers(Utils::PtrList *members);
};

class EnumType : public Type {};

class FuncType : public DerivedType {
    private:
        /**< list of decl* */
        Utils::PtrList m_params;
        bool           m_vaarg;
    public:
        FuncType(QualType ret, Utils::PtrList &&params, bool vaarg) noexcept;
        
        FuncType*       toFunc()       noexcept override;
        const FuncType* toFunc() const noexcept override;
        
        bool isCompatible(Type*) noexcept override;
        
        Text::UString toString() const override;
        
        QualType returnType() const noexcept;
        void setReturnType(QualType) noexcept;
        
        bool isVaArgs() const noexcept;
        void setVaArgs(bool) noexcept;
        
        Utils::PtrList& params() noexcept;
        void setParams(Utils::PtrList &&) noexcept;
};

VoidType*    makeVoidType();
NumberType*  makeNumberType(uint32_t spec);
PointerType* makePointerType(QualType base);
PointerType* makePointerType(Type *base, uint32_t qual = 0);
ArrayType*   makeArrayType(QualType base, int bound = -1);
StructType*  makeStructType(Utils::PtrList *members = nullptr);
FuncType*    makeFuncType(QualType ret, Utils::PtrList &&params, bool vaarg);

} // namespace Semantic
} // namespace Compiler

#endif // TYPE_HPP
