#include "text/ustring.hpp"
#include "diagnostic/printer.hpp"
#include "lexical/tokentype.hpp"
#include "semantic/typeenum.hpp"

#include <cassert>

namespace Compiler {
namespace Diagnostic {
struct SourceLoc;
}
namespace Semantic {
extern Diagnostic::SourceLoc *epos;
}
}

namespace impl = Compiler::Semantic;

using namespace Compiler::Text;
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

Specifier impl::toSpecifier(TokenType token) noexcept {
    switch(token) {
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
    }
    assert(false);
}

const char* impl::toString(Qualifier qual) noexcept {
    switch(qual) {
        case Const: return "const";
        case Volatile: return "volatile";
        case Restrict: return "restrict";
    }
    assert(false);
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
    }
    assert(false);
}

const char* impl::toString(StorageClass stor) noexcept {
    switch(stor) {
        case Typedef: return "typedef";
        case Static: return "static";
        case Inline: return "inline";
        case Register: return "register";
        case Extern: return "extern";
    }
}

UString impl::specifierToString(uint32_t spec) {
    UString ret{};
    uint32_t mask = 1;
    bool space = false;
    while(spec) {
        if(spec & mask) {
            spec ^= mask;
            if(space) 
                ret += ' ';
            ret += toString(static_cast<Specifier>(mask));
            space = true;
        }
        mask <<= 1;
    }
    return ret;
}

/* C99 6.7.3 Type qualifiers
 * 
 * If the same qualifier appears more than once in the same specifier-qualifier-list, either
 * directly or via one or more typedefs, the behavior is the same as if it appeared only
 * once.
 */
uint32_t impl::addQualifier(uint32_t lhs, uint32_t rhs) noexcept {
    if(lhs & rhs) 
        Printer(DIAGNOSTIC_WARNING) << epos
            << "Duplicate qualifier " << toString(static_cast<Qualifier>(rhs));
    return lhs |= rhs;
}

uint32_t impl::addStorageClass(uint32_t lhs, uint32_t rhs) noexcept {
    static constexpr uint32_t comp[] = { // Compatibility mask
        0, // Typedef
        Inline, // Static
        Static, // Inline
        0, // Register
        0, // Extern
    };
    if(lhs & ~comp[offset(rhs)]) 
        Printer(DIAGNOSTIC_ERROR) << epos 
            << "Cannot apply storage class specifier " << toString(static_cast<StorageClass>(rhs)) 
            << " to previous one";
    else if(rhs & Register) 
        Printer(DIAGNOSTIC_WARNING) << epos
            << "Deprecated storage class specifier \"register\", it has no effect";
    return lhs |= rhs;
}

uint32_t impl::addSpecifier(uint32_t lhs, uint32_t rhs) noexcept {
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
    
    // incompatible two specifiers
    if(lhs & ~comp[offset(rhs)])
        Printer(DIAGNOSTIC_ERROR) << epos
            << "Cannot apply specifier "<< toString(static_cast<Specifier>(rhs))
            << "to specifier sequence " << specifierToString(lhs);
    
    if((lhs & Long) && (rhs & Long)) {
        lhs ^= Long;
        lhs |= LLong;
    } else 
        lhs |= rhs;
    
    return lhs;
}
