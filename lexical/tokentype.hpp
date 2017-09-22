#ifndef TOKENTYPE_HPP
#define TOKENTYPE_HPP

#include <cstdint>

namespace Compiler {

namespace Lexical {

enum TokenType : uint32_t {
    Error = 0xffffffff,
    Eof = Error - 1, // end of file
    
    Identifier = 0x10000000,
    Constant,
    Character = 0x04000000,
    WideCharacter = 0x04000001,
    String = 0x08000000,
    WideString = 0x08000001,
    PPNumber, // PreProcessing number
    PPFloat,
    
    Space,
    Newline,
    
    // Operators
    StringConcat = 0, // ##
    Escape,           // '\\'
    // Operators with ambiguity
    Dot,              // .
    Star,             // *
    Ampersand,        // &
    Pound,            // #
    // normal
    Ellipsis,         // ...
    Semicolon,        // ;
    BlockOpen,        // {
    BlockClose,       // }
    LeftParen,        // (
    RightParen,       // )
    LeftSubscript,    // [
    RightSubscript,   // ]
    Question,         // ?
    Colon,            // :
    MemberPtr,        // ->
    Comma,            // ,
    Add,              // +
    Sub,              // -
    Div,              // /
    Mod,              // %
    BitOr,            // |
    BitXor,           // ^
    BitNot,           // ~
    LeftShift,        // <<
    RightShift,       // >>
    LessThan,         // <
    LessEqual,        // <=
    GreaterThan,      // >
    GreaterEqual,     // >=
    Equal,            // ==
    NotEqual,         // !=
    LogicalAnd,       // &&
    LogicalOr,        // ||
    LogicalNot,       // !
    // Operator with assignments
    Inc,              // ++
    Dec,              // --
    Assign,           // =
    AddAssign,        // +=
    SubAssign,        // -=
    MulAssign,        // *=
    DivAssign,        // /=
    ModAssign,        // %=
    BitAndAssign,     // &=
    BitOrAssign,      // |=
    BitXorAssign,     // ^=
    LeftShiftAssign,  // <<=
    RightShiftAssign, // >>=
    
    // Keywords: storage specifier
    KeyStatic = 0x01000000,
    KeyAuto, // TODO: reversed for type deduction semantic
    KeyRegister,
    KeyExtern,
    KeyInline,
    KeyTypedef,
    // Keywords: type qualifier
    KeyConst = 0x01100000,
    KeyVolatile,
    KeyRestrict,
    // Keywords: type specifier
    KeyBool = 0x01200000,
    KeyComplex,
    KeyChar,
    KeyDouble,
    KeyEnum,
    KeyFloat,
    KeyImaginary,
    KeyInt,
    KeyLong,
    KeySigned,
    KeyShort,
    KeyStruct,
    KeyUnion,
    KeyUnsigned,
    KeyVoid,
    // Keywords: control flow
    KeyBreak = 0x01400000,
    KeyCase,
    KeyContinue,
    KeyDefault,
    KeyDo,
    KeyIf,
    KeyElse,
    KeyFor,
    KeyGoto,
    KeyReturn,
    KeySwitch,
    KeyWhile,
    // Keywords: operator
    KeySizeof = 0x01800000,
    //KeyDecltype, // TODO
    // Keywords: literal
    KeyTrue,
    KeyFalse,
    
    // Directives
    DirectInclude = 0x02000000,
    DirectDefine,
    DirectUndef,
    DirectDefined, // Actually an operator
    //DirectIf, // conflicts with KeyIf
    //DirectElse, // conflicts with KeyElse
    DirectIfdef,
    DirectIfndef,
    DirectElif,
    DirectEndif,
    DirectLine,
    DirectError,
    DirectPragma,
    // Directives: miscellaneous
    DirectVAArgs,
};

bool isAssignment(TokenType) noexcept;
bool isStorageClass(TokenType) noexcept;
bool isTypeSpecifier(TokenType) noexcept;
bool isQualifier(TokenType) noexcept;

const char* toString(TokenType type) noexcept;

} // namespace Lexical
} // namespace Compiler

#endif // TOKENTYPE_HPP
