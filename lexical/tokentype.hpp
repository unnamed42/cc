#ifndef TOKENTYPE_HPP
#define TOKENTYPE_HPP

#include <cstdint>

namespace Compiler {

namespace Lexical {

enum TokenType : uint32_t {
    Error = 0xffffffff,
    Eof = 0, // end of file
    
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
    #define OP1(o1) (o1)
    #define OP2(o2, o1) (((o2) << 8) | (o1))
    #define OP3(o3, o2, o1) (((o3) << 16) | ((o2) << 8) | (o1))
    StringConcat = OP2('#', '#'),           // ##
    Escape = OP1('\\'),                     // '\\'
    // Operators with ambiguity
    Dot = OP1('.'),                         // .
    Star = OP1('*'),                        // *
    Ampersand = OP1('&'),                   // &
    Pound = OP1('#'),                       // #
    // normal
    Ellipsis = OP3('.','.','.'),            // ...
    Semicolon = OP1(';'),                   // ;
    BlockOpen = OP1('{'),                   // {
    BlockClose = OP1('}'),                  // }
    LeftParen = OP1('('),                   // (
    RightParen = OP1(')'),                  // )
    LeftSubscript = OP1('['),               // [
    RightSubscript = OP1(']'),              // ]
    Question = OP1('?'),                    // ?
    Colon = OP1(':'),                       // :
    MemberPtr = OP2('-', '>'),              // ->
    Comma = OP1(','),                       // ,
    Add = OP1('+'),                         // +
    Sub = OP1('-'),                         // -
    Div = OP1('/'),                         // /
    Mod = OP1('%'),                         // %
    BitOr = OP1('|'),                       // |
    BitXor = OP1('^'),                      // ^
    BitNot = OP1('~'),                      // ~
    LeftShift = OP2('<', '<'),              // <<
    RightShift = OP2('>', '>'),             // >>
    LessThan = OP1('<'),                    // <
    LessEqual = OP2('<', '='),              // <=
    GreaterThan = OP1('>'),                 // >
    GreaterEqual = OP2('>', '='),           // >=
    Equal = OP2('=', '='),                  // ==
    NotEqual = OP2('!', '='),               // !=
    LogicalAnd = OP2('&', '&'),             // &&
    LogicalOr = OP2('|', '|'),              // ||
    LogicalNot = OP1('!'),                  // !
    // Operator with assignments
    Inc = OP2('+', '+'),                    // ++
    Dec = OP2('-', '-'),                    // --
    Assign = OP1('='),                      // =
    AddAssign = OP2('+', '='),              // +=
    SubAssign = OP2('-', '='),              // -=
    MulAssign = OP2('*', '='),              // *=
    DivAssign = OP2('/', '='),              // /=
    ModAssign = OP2('%', '='),              // %=
    BitAndAssign = OP2('&', '='),           // &=
    BitOrAssign = OP2('|', '='),            // |=
    BitXorAssign = OP2('^', '='),           // ^=
    LeftShiftAssign = OP3('<', '<', '='),   // <<=
    RightShiftAssign = OP3('>', '>', '='),  // >>=
    #undef OP1
    #undef OP3
    #undef OP2
    
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

TokenType toTokenType(char op) noexcept;

const char* toString(TokenType type) noexcept;

} // namespace Lexical
} // namespace Compiler

#endif // TOKENTYPE_HPP
