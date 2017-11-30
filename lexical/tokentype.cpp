#include "tokentype.hpp"

namespace impl = Compiler::Lexical;

using namespace Compiler::Lexical;

#define BETWEEN(value, left, right) \
    (left) <= (value) && (value) <= (right)

bool impl::isAssignment(TokenType type) noexcept {
    return BETWEEN(type, Assign, RightShiftAssign);
} 

bool impl::isStorageClass(TokenType type) noexcept {
    return BETWEEN(type, KeyStatic, KeyTypedef);
}

bool impl::isTypeSpecifier(TokenType type) noexcept {
    return BETWEEN(type, KeyBool, KeyVoid);
}

bool impl::isQualifier(TokenType type) noexcept {
    return BETWEEN(type, KeyConst, KeyRestrict);
}

const char* impl::toString(TokenType type) noexcept {
    switch(type) {
        case Error: return "error";
        case Eof: return  "eof";
            
        case Identifier: return "identifier"; 
        case Constant: return "constant";
        case Character: return "char";
        case WideCharacter: return "wchar";
        case String: return "string";
        case WideString: return "wstring";
        case PPNumber: return "number";
        case PPFloat: return "float";
            
        case Space: return " ";
        case Newline: return "\n";

        case StringConcat: return "##";
        case Escape: return "\\";
    
        case Dot: return ".";
        case Star: return "*";
        case Ampersand: return "&";  
        case Pound: return "#";
    
        case Ellipsis: return "...";
        case Semicolon: return ";";
        case BlockOpen: return "{"; 
        case BlockClose: return "}";
        case LeftParen: return "(";
        case RightParen: return ")";
        case LeftSubscript: return "[";
        case RightSubscript: return "]";
        case Question: return "?";
        case Colon: return ":";
        case MemberPtr: return "->"; 
        case Comma: return ",";
        case Add: return "+";
        case Sub: return "-";
        case Div: return "/";
        case Mod: return "%";
        case BitOr: return "|";
        case BitXor: return "^";
        case BitNot: return "~";
        case LeftShift: return "<<"; 
        case RightShift: return ">>";
        case LessThan: return "<"; 
        case LessEqual: return "<="; 
        case GreaterThan: return ">"; 
        case GreaterEqual: return ">="; 
        case Equal: return "==";
        case NotEqual: return "!=";
        case LogicalAnd: return "&&";
        case LogicalOr: return "||"; 
        case LogicalNot: return "!"; 
    
        case Inc: return "++";
        case Dec: return "--";
        case Assign: return "="; 
        case AddAssign: return "+="; 
        case SubAssign: return "-=";
        case MulAssign: return "*="; 
        case DivAssign: return "/="; 
        case ModAssign: return "%="; 
        case BitAndAssign: return "&="; 
        case BitOrAssign: return "|="; 
        case BitXorAssign: return "^="; 
        case LeftShiftAssign: return "<<="; 
        case RightShiftAssign: return ">>="; 
    
        case KeyStatic: return "static";
        case KeyAuto: return "auto"; 
        case KeyRegister: return "register";
        case KeyExtern: return "extern";
        case KeyInline: return "inline";
        case KeyTypedef: return "typedef";
    
        case KeyConst: return "const";
        case KeyVolatile: return "volatile";
        case KeyRestrict: return "restrict";
    
        case KeyBool: return "bool";
        case KeyComplex: return "complex";
        case KeyChar: return "char";
        case KeyDouble: return "double";
        case KeyEnum: return "enum";
        case KeyFloat: return "float";
        case KeyImaginary: return "imaginary";
        case KeyInt: return "int";
        case KeyLong: return "long";
        case KeySigned: return "signed";
        case KeyShort: return "short";
        case KeyStruct: return "struct";
        case KeyUnion: return "union";
        case KeyUnsigned: return "unsigned";
        case KeyVoid: return "void";
    
        case KeyBreak: return "break";
        case KeyCase: return "case";
        case KeyContinue: return "continue";
        case KeyDefault: return "default";
        case KeyDo: return "do";
        case KeyIf: return "if";
        case KeyElse: return "else";
        case KeyFor: return "for";
        case KeyGoto: return "goto";
        case KeyReturn: return "return";
        case KeySwitch: return "switch";
        case KeyWhile: return "while";
    
        case KeySizeof: return "sizeof";
    
        case KeyTrue: return "true";
        case KeyFalse: return "false";
    
        case DirectInclude: return "include";
        case DirectDefine: return "define";
        case DirectUndef: return "undef";
        case DirectDefined: return "defined";
    
        case DirectIfdef: return "ifdef";
        case DirectIfndef: return "ifndef";
        case DirectElif: return "elif";
        case DirectEndif: return "endif";
        case DirectLine: return "line";
        case DirectError: return "error";
        case DirectPragma: return "pragma";
    
        case DirectVAArgs: return "__VA_ARGS__";
    }
}
