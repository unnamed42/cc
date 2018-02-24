#ifndef __COMPILER_TOKEN__
#define __COMPILER_TOKEN__

#include "error.hpp" // file_pos

#include <list>
#include <string>
#include <cstdint>

namespace compiler {

enum token_attr: uint32_t {
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
    
    Newline,
    
    If, // has ambiguity: keyword "if" and directive "if"
    Else, // same problem with If
    
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
    KeyVolatile = 0x01100000,
    KeyConst,
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
    //KeyIf,
    //KeyElse,
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
    // Keywords: miscellaneous
    KeyVAArgs,
    
    // Directives
    DirectInclude = 0x02000000,
    DirectDefine,
    DirectUndef,
    DirectDefined, // Actually an operator
    //DirectIf,
    //DirectElse,
    DirectIfdef,
    DirectIfndef,
    DirectElif,
    DirectEndif,
    DirectLine,
    DirectError,
    DirectPragma,
};

bool is_operator(uint32_t);
bool is_assignment(uint32_t);

bool is_keyword(uint32_t);
bool is_storage_specifier(uint32_t);
bool is_type_specifier(uint32_t);
bool is_type_qualifier(uint32_t);

bool is_directive(uint32_t);

uint32_t    string_to_attr(const std::string&);
const char* attr_to_string(uint32_t);

// insert a string to global string table
const char* insert_string(const std::string&);

struct token {
    uint32_t    m_attr;
    file_pos    m_pos;
    const char *m_str;  // source text
    
    token(uint32_t, const file_pos&, const char* = nullptr);
    
    const char* to_string() const {return m_str;}
    bool is(uint32_t a) const {return m_attr == a;}
};

token* make_token(uint32_t, const file_pos&); // used for delimiters
token* make_token(uint32_t, const file_pos&, std::string&); // used for identifiers, keywords, constants

typedef std::list<token*>   token_list;

// global token pointer used as error message locator
extern const file_pos *epos;

inline void mark_pos(const token *tok) {epos = &tok->m_pos;}

} // namespace compiler 
#endif // __COMPILER_TOKEN__
