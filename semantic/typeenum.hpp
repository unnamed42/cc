#ifndef TYPEENUM_HPP
#define TYPEENUM_HPP

#include <cstdint>

namespace Compiler {
    
namespace Lexical {
enum TokenType : uint32_t;
}

namespace Semantic {
enum Specifier : uint32_t {
    Void = 0x01,
    Bool = Void << 1,
    Char = Bool << 1,
    Short = Char << 1,
    Int = Short << 1,
    Long = Int << 1,
    LLong = Long << 1,
    Float = LLong << 1,
    Double = Float << 1,
    // long double  == Long | Double
    Complex = Double << 1,
    Unsigned = Complex << 1,
    Signed   = Unsigned << 1,
    
    Base = 
        Void | Bool | Char | Short | Int | Long | LLong | 
        Float | Double | Complex | Signed | Unsigned,
    Sign = 
        Signed | Unsigned,
    Integer = 
        Bool | Char | Short | Int | Long | LLong | Signed | Unsigned,
    Floating = // better name?
        Float | Double,
};

enum Qualifier : uint32_t {
    Const = 1,
    Volatile = Const << 1,
    Restrict = Volatile << 1, // pointer only
    Qual = 
        Const | Volatile | Restrict,
};

enum StorageClass: uint32_t {
    Auto = 0, // TODO: reversed keyword used for type deduction
    Typedef = 1,
    Static = Typedef << 1,
    Inline = Static << 1,
    Register = Inline << 1,
    Extern = Register << 1,
};

enum TypeSize : uint32_t {
    SizeBool = 1,
    SizeChar = 1,
    SizeShort = 2,
    SizeInt = 4,
    SizeLong = 4,
    SizeLLong = 8,
    
    SizeFloat = 4,
    SizeDouble = 8,
    SizeLDouble = 8,
    
    SizePointer = 4,
};

Specifier    toSpecifier(Lexical::TokenType) noexcept;
StorageClass toStorageClass(Lexical::TokenType) noexcept;
Qualifier    toQualifier(Lexical::TokenType) noexcept;

const char* toString(Qualifier) noexcept;
const char* toString(Specifier) noexcept;
const char* toString(StorageClass) noexcept;

uint32_t sizeOf(uint32_t spec) noexcept;

uint32_t addQualifier(uint32_t lhs, uint32_t rhs) noexcept;
uint32_t addStorageClass(uint32_t lhs, uint32_t rhs) noexcept;
uint32_t addSpecifier(uint32_t lhs, uint32_t rhs) noexcept;

} // namespace Semantic
} // namespace Compiler

#endif // TYPEENUM_HPP

