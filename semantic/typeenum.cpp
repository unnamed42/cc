#include "lexical/token.hpp"
#include "diagnostic/logger.hpp"
#include "lexical/tokentype.hpp"
#include "semantic/typeenum.hpp"

#include <cassert>

namespace impl = Compiler::Semantic;

using namespace Compiler::Lexical;
using namespace Compiler::Semantic;
using namespace Compiler::Diagnostic;

static inline int offset(uint32_t data) noexcept {
    unsigned int i = 0;
    while(data >>= 1)
        ++i;
    return i;
}

uint32_t impl::sizeOf(uint32_t spec) noexcept {
    switch(spec) {
        case Bool: return SizeBool;
        case Char: case Signed|Char: case Unsigned|Char: 
            return SizeChar;
        case Short: case Signed|Short:
        case Short|Int: case Signed|Short|Int:
        case Unsigned|Short: case Unsigned|Short|Int: 
            return SizeShort;
        case Int: case Signed: case Signed|Int:
        case Unsigned: case Unsigned|Int: 
            return SizeInt;
        case Long: case Signed|Long:
        case Long|Int: case Signed|Long|Int:
        case Unsigned|Long: case Unsigned|Long|Int: 
            return SizeLong;
        case LLong: case Signed|LLong:
        case LLong|Int: case Signed|LLong|Int:
        case Unsigned|LLong: case Unsigned|LLong|Int:
            return SizeLLong;
        case Float: return SizeFloat;
        case Double: return SizeDouble;
        case Long|Double: return SizeLDouble;
    }
    assert(false);
}

Specifier impl::toSpecifier(Token *tok) noexcept {
    switch(tok->type()) {
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
        default: assert(false);
    }
}

Qualifier impl::toQualifier(Token *tok) noexcept {
    switch(tok->type()) {
        case KeyConst: return Const;
        case KeyVolatile: return Volatile;
        case KeyRestrict: return Restrict;
        default: assert(false);
    }
}

StorageClass impl::toStorageClass(Token *tok) noexcept {
    switch(tok->type()) {
        case KeyStatic: return Static;
        case KeyAuto: return Auto;
        case KeyRegister: return Register;
        case KeyExtern: return Extern;
        case KeyInline: return Inline;
        case KeyTypedef: return Typedef;
        default: assert(false);
    }
}

const char* impl::toString(Qualifier qual) noexcept {
    switch(qual) {
        case Const: return "const";
        case Volatile: return "volatile";
        case Restrict: return "restrict";
        default: assert(false);
    }
}

const char* impl::toString(Specifier type) noexcept {
    switch(type) {
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
        default: assert(false);
    }
}

const char* impl::toString(StorageClass stor) noexcept {
    switch(stor) {
        case Auto: return "";
        case Typedef: return "typedef";
        case Static: return "static";
        case Inline: return "inline";
        case Register: return "register";
        case Extern: return "extern";
        default: assert(false);
    }
}

/* C99 6.7.3 Type qualifiers
 * 
 * If the same qualifier appears more than once in the same specifier-qualifier-list, either
 * directly or via one or more typedefs, the behavior is the same as if it appeared only
 * once.
 */
uint32_t impl::addQualifier(uint32_t lhs, Token *rhsTok) noexcept {
    auto rhs = toQualifier(rhsTok);
    if(lhs & rhs) 
        dwarn << rhsTok->sourceLoc() << "duplicate qualifier " << static_cast<Qualifier>(rhs);
    return lhs |= rhs;
}

uint32_t impl::addStorageClass(uint32_t lhs, Token *rhsTok) noexcept {
    static constexpr uint32_t comp[] = { // Compatibility mask
        0, // Typedef
        Inline, // Static
        Static, // Inline
        0, // Register
        0, // Extern
    };
    auto rhs = toStorageClass(rhsTok);
    if(lhs & ~comp[offset(rhs)]) 
        derr << rhsTok->sourceLoc() << "cannot apply storage class specifier '" 
            << static_cast<StorageClass>(rhs)
            << "' to '" << Logger::storageClasses << lhs << '\'';
    else if(rhs & Register) 
        derr << rhsTok->sourceLoc() << "deprecated storage class specifier 'register', it will has no effect";
    return lhs |= rhs;
}

uint32_t impl::addSpecifier(uint32_t lhs, Token *rhsTok) noexcept {
    static constexpr uint32_t comp[12] = { // Compatibility masks
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
    auto rhs = toSpecifier(rhsTok);
    // incompatible two specifiers
    if(lhs & ~comp[offset(rhs)])
        derr << rhsTok->sourceLoc()
            << "cannot apply specifier '" << static_cast<Specifier>(rhs)
            << "' to specifier sequence '" << Logger::specifiers << lhs << '\'';
    
    if((lhs & Long) && (rhs & Long)) {
        lhs ^= Long;
        lhs |= LLong;
    } else 
        lhs |= rhs;
    
    return lhs;
}
