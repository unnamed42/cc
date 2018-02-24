#include "token.hpp"
#include "lexer.hpp"
#include "mempool.hpp"

#include <unordered_set>
#include <unordered_map>

using namespace compiler;

static mempool<token> token_pool{};

const file_pos* compiler::epos = nullptr;

static constexpr auto operator_mask  = 0xff000000U; // requires negated
static constexpr auto keyword_mask   = 0x01000000U;
static constexpr auto directive_mask = 0x02000000U;
static constexpr auto flag_mask      = 0xfff00000U;
static constexpr auto storage_flag   = KeyStatic & flag_mask;
static constexpr auto qualifier_flag = KeyConst  & flag_mask;
static constexpr auto specifier_flag = KeyLong   & flag_mask;

bool compiler::is_operator(uint32_t attr) {
    return !(attr & operator_mask);
}

bool compiler::is_assignment(uint32_t attr) {
    return is_operator(attr) && (attr & 0xff) == '=';
}

bool compiler::is_keyword(uint32_t attr) {
    return attr & keyword_mask;
}

bool compiler::is_storage_specifier(uint32_t attr) {
    return is_keyword(attr) && (attr & flag_mask) == storage_flag;
}

bool compiler::is_type_qualifier(uint32_t attr) {
    return is_keyword(attr) && (attr & flag_mask) == qualifier_flag;
}

bool compiler::is_type_specifier(uint32_t attr) {
    return is_keyword(attr) && 
           (((attr & flag_mask) == specifier_flag) || attr == Identifier); // potential typedef
}

bool compiler::is_directive(uint32_t attr) {
    return attr & directive_mask;
}

uint32_t compiler::string_to_attr(const std::string &str) {
    static const std::unordered_map<std::string,token_attr> str_attr {
        {"auto", KeyAuto},
        {"break", KeyBreak},
        {"bool", KeyBool}, // C++ standard here
        {"case", KeyCase},
        {"char", KeyChar},
        {"const", KeyConst},
        {"continue", KeyContinue},
        {"default", KeyDefault},
        {"do", KeyDo},
        {"double", KeyDouble},
        {"else", Else},
        {"enum", KeyEnum},
        {"extern", KeyExtern},
        {"false", KeyFalse}, // C++
        {"float", KeyFloat},
        {"for", KeyFor},
        {"goto", KeyGoto},
        {"if", If},
        {"inline", KeyInline},
        {"int", KeyInt},
        {"long", KeyLong},
        {"register", KeyRegister},
        {"restrict", KeyRestrict},
        {"return", KeyReturn},
        {"short", KeyShort},
        {"signed", KeySigned},
        {"sizeof", KeySizeof},
        {"static", KeyStatic},
        {"struct", KeyStruct},
        {"switch", KeySwitch},
        {"typedef", KeyTypedef},
        {"true", KeyTrue}, // C++
        {"union", KeyUnion},
        {"unsigned", KeyUnsigned},
        {"void", KeyVoid},
        {"volatile", KeyVolatile},
        {"while", KeyWhile},
        {"_Complex", KeyComplex},
        {"_Imaginary", KeyImaginary},   
        
        {"__VA_ARGS__", KeyVAArgs},
        
        {"include", DirectInclude},
        {"define", DirectDefine},
        {"defined", DirectDefined},
        {"ifdef", DirectIfdef},
        {"ifndef", DirectIfndef},
        {"elif", DirectElif},
        {"endif", DirectEndif},
        {"line", DirectLine},
        {"error", DirectError},
        {"pragma", DirectPragma},    
    };
    
    auto it = str_attr.find(str);
    return it != str_attr.cend() ? 
           it->second :
           Error;
}

const char* compiler::attr_to_string(uint32_t attr) {
    static const std::unordered_map<uint32_t, std::string> attr_str {
        {Newline, "\n"},
        
        {If, "if"},
        {Else, "else"},
        
        {Pound, "#"},
        {StringConcat, "##"},
        {Escape, "\\"},
        {Dot, "."},
        {Star, "*"},
        {Ampersand, "&"},
        {Ellipsis, "..."},
        {Semicolon, ";"},
        {BlockOpen, "{"},
        {BlockClose, "}"},
        {LeftParen, "("},
        {RightParen, ")"},
        {LeftSubscript, "["},
        {RightSubscript, "]"},
        {Question, "?"},
        {Colon, ":"},
        {MemberPtr, "->"},
        {Comma, ","},
        {Add, "+"},
        {Sub, "-"},
        {Div, "/"},
        {Mod, "%"},
        {BitOr, "|"},
        {BitXor, "^"},
        {BitNot, "~"},
        {LeftShift, "<<"},
        {RightShift, ">>"},
        {LessThan, "<"},
        {LessEqual, "<="},
        {GreaterThan, ">"},
        {GreaterEqual, ">="},
        {Equal, "=="},
        {NotEqual, "!="},
        {LogicalAnd, "&&"},
        {LogicalOr, "||"},
        {LogicalNot, "!"},
        {Inc, "++"},
        {Dec, "--"},
        {Assign, "="},
        {AddAssign, "+="},
        {SubAssign, "-="},
        {MulAssign, "*="},
        {DivAssign, "="},
        {ModAssign, "%="},
        {BitAndAssign, "&="},
        {BitOrAssign, "|="},
        {BitXorAssign, "^="},
        {LeftShiftAssign, "<<="},
        {RightShiftAssign, ">>="},
        
        {KeyAuto, "auto"},
        {KeyBreak, "break"},
        {KeyCase, "case"},
        {KeyChar, "char"},
        {KeyConst, "const"},
        {KeyContinue, "continue"},
        {KeyDefault, "default"},
        {KeyDo, "do"},
        {KeyDouble, "double"},
        //{KeyIf, "if"},
        //{KeyElse, "else"},
        {KeyTrue, "true"},
        {KeyFalse, "false"},
        {KeyEnum, "enum"},
        {KeyExtern, "extern"},
        {KeyFloat, "float"},
        {KeyFor, "for"},
        {KeyGoto, "goto"},
        {KeyInline, "inline"},
        {KeyInt, "int"},
        {KeyLong, "long"},
        {KeyRegister, "register"},
        {KeyRestrict, "restrict"},
        {KeyReturn, "return"},
        {KeyShort, "short"},
        {KeySigned, "signed"},
        {KeySizeof, "sizeof"},
        {KeyStatic, "static"},
        {KeyStruct, "struct"},
        {KeySwitch, "switch"},
        {KeyTypedef, "typedef"},
        {KeyUnion, "union"},
        {KeyUnsigned, "unsigned"},
        {KeyVoid, "void"},
        {KeyVolatile, "volatile"},
        {KeyWhile, "while"},
        {KeyBool, "bool"},
        {KeyComplex, "_Complex"},
        {KeyImaginary, "_Imaginary"},   
        
        {KeyVAArgs, "__VA_ARGS__"},
        
        {DirectInclude, "include"},
        {DirectDefine, "define"},
        {DirectDefined, "defined"},
        //{DirectIf, "if"},
        //{DirectElse, "else"},
        {DirectIfdef, "ifdef"},
        {DirectIfndef, "ifndef"},
        {DirectElif, "elif"},
        {DirectEndif, "endif"},
        {DirectLine, "line"},
        {DirectError, "error"},
        {DirectPragma, "pragma"},   
    };
    
    auto it = attr_str.find(attr);
    return it == attr_str.cend() ?
           "" :
           it->second.c_str();
}

const char* compiler::insert_string(const std::string &str) {
    static std::unordered_set<std::string> set{};
    return set.insert(str).first->c_str();
}

token::token(uint32_t attr, const file_pos &loc, const char *str):
    m_attr(attr), m_pos(loc), m_str(str) {}

token* compiler::make_token(uint32_t attr, const file_pos &pos) {
    return new (token_pool.malloc()) token(attr, pos, attr_to_string(attr));
}

token* compiler::make_token(uint32_t attr, const file_pos &pos, std::string &s) {
    return new (token_pool.malloc()) token(attr, pos, insert_string(s));
}
