#include "utils/mempool.hpp"
#include "text/uchar.hpp"
#include "text/ustring.hpp"
#include "lexical/tokentype.hpp"
#include "lexical/token.hpp"
#include "lexical/lexer.hpp"
#include "diagnostic/sourceloc.hpp"
#include "diagnostic/printer.hpp"

#include <cassert>

using namespace Compiler::Text;
using namespace Compiler::Utils;
using namespace Compiler::Lexical;
using namespace Compiler::Diagnostic;

static inline bool isOneOf(char c, const char *str) noexcept {
    while(*str) {
        if(*(str++) == c)
            return true;
    }
    return false;
}

Lexer::Lexer(const char *path) : m_src(path), m_pos(1) {}

// Lexer::Lexer(const UString &src)
//     : m_src(src.c_str(), nullptr) {}

Token* Lexer::makeToken(TokenType type) {
    // TODO: location fix
    auto loc = makeSourceLoc(&m_src.sourceLoc());
    loc->length = m_src.pos() - m_pos;
    loc->column -= loc->length;
    return ::makeToken(loc, type);
}

Token* Lexer::makeToken(TokenType type, UString &content) {
    auto loc = makeSourceLoc(&m_src.sourceLoc());
    loc->length = m_src.pos() - m_pos;
    loc->column -= loc->length;
    return ::makeToken(loc, type, reinterpret_cast<UString*>(content.toHeap()));
}

void Lexer::logPos() {
    m_pos = m_src.pos();
}

const SourceLoc* Lexer::sourceLoc() const noexcept {
    return &m_src.sourceLoc();
}

Token* Lexer::expect(TokenType type) {
    auto ret = get();
    if(!ret->is(type)) {
        derr << ret->sourceLoc()
            << "expecting '" << toString(type) << "', but get '"
            << toString(ret->type()) << '\'';
    }
    return ret;
}

Token* Lexer::get() {
    logPos();
    
    switch(m_src.skipSpace()) {
        case 1: return makeToken(Space);
        case 2:
        case 3: return makeToken(Newline);
        case 0:
        default: break;
    }
    
    auto ch = m_src.get();
    
    Token* temp;
    
    // parsing delimiter using greedy policy
    switch(ch) {
        case '\0': return makeToken(Eof);
        case '0' ... '9': return getNumber(ch);
        case 'L':
            if(m_src.want('\'')) return getChar();
            if(m_src.want('\"')) return getString();
        case 'a' ... 'z': case 'A' ... 'K': case 'M' ... 'Z':
        case '_': case '$': case 0x80000000 ... 0xffffffff:
            return getIdentifier(ch);
        case '\'': return getChar();
        case '\"': return getString();
        case '\\':
            if(m_src.want('u')) return getIdentifier(getUCN(4));
            if(m_src.want('U')) return getIdentifier(getUCN(8));
            return makeToken(Escape);
        case '=': return makeToken(m_src.want('=') ? Equal : Assign);
        case '+':
            if(m_src.want('+')) return makeToken(Inc);
            if(m_src.want('=')) return makeToken(AddAssign);
            return makeToken(Add);
        case '-':
            if(m_src.want('-')) return makeToken(Dec);
            if(m_src.want('=')) return makeToken(SubAssign);
            if(m_src.want('>')) return makeToken(MemberPtr);
            return makeToken(Sub);
        case '*': return makeToken(m_src.want('=') ? MulAssign : Star);
        case '/': return makeToken(m_src.want('=') ? DivAssign : Div);
        case '%':
            temp = getDigraph(ch);
            if(temp) return temp;
            if(m_src.want('=')) return makeToken(ModAssign);
            return makeToken(Mod);
        case '&':
            if(m_src.want('&')) return makeToken(LogicalAnd);
            if(m_src.want('=')) return makeToken(BitAndAssign);
            return makeToken(Ampersand);
        case '|':
            if(m_src.want('|')) return makeToken(LogicalOr);
            if(m_src.want('=')) return makeToken(BitOrAssign);
            return makeToken(BitOr);
        case '^': return makeToken(m_src.want('=') ? BitXorAssign : BitXor);
        case '~': return makeToken(BitNot);
        case '!': return makeToken(m_src.want('=') ? NotEqual : LogicalNot);
        case '<':
            temp = getDigraph(ch);
            if(temp) return temp;
            if(m_src.want('<')) return makeToken(m_src.want('=') ? LeftShiftAssign : LeftShift);
            return makeToken(m_src.want('=') ? LessEqual : LessThan);
        case '>':
            if(m_src.want('>')) return makeToken(m_src.want('=') ? RightShiftAssign : RightShift);
            return makeToken(m_src.want('=') ? GreaterEqual : GreaterThan);
        case ':':
            temp = getDigraph(ch);
            if(temp) return temp;
            return makeToken(Colon);
        case '#': return makeToken(m_src.want('#') ? StringConcat : Pound);
        case '(': return makeToken(LeftParen);
        case ')': return makeToken(RightParen);
        case ',': return makeToken(Comma);
        case ';': return makeToken(Semicolon);
        case '[': return makeToken(LeftSubscript);
        case ']': return makeToken(RightSubscript);
        case '{': return makeToken(BlockOpen);
        case '}': return makeToken(BlockClose);
        case '?': return makeToken(Question);
        case '.':
            if(m_src.peek().isNumber()) return getNumber(ch);
            if(m_src.want('.')) {
                if(m_src.want('.'))
                    return makeToken(Ellipsis);
                m_src.unget();
            }
            return makeToken(Dot);
        default: derr << sourceLoc() << ch;
    }
}

/* C99 6.4.6 Punctuators
 *
 * In all aspects of the language, the six Tokens
 * <: :> <% %> %: %:%:
 * behave, respectively, the same as the six Tokens
 * [  ]  {  }  #  ##
 * except for their spelling.
 */
// digraphs are recognized during lexical analysis
Token* Lexer::getDigraph(UChar ch) {
    switch(ch) {
        case '<':
            if(m_src.want(':')) return makeToken(LeftSubscript);
            if(m_src.want('%')) return makeToken(BlockOpen);
            return nullptr;
        case ':': return m_src.want('>') ? makeToken(RightSubscript) : nullptr;
        case '%':
            if(m_src.want('>')) return makeToken(BlockClose);
            if(m_src.want(':')) {
                if(m_src.want('%'))  {
                    if(m_src.want(':')) return makeToken(StringConcat);
                    m_src.unget();
                }
                return makeToken(Pound);
            }
        default: return nullptr;
    }
}

Token* Lexer::getNumber(UChar ch) {
    bool maybe_float = (ch == '.');
    UString ret{ch, 1};
    auto last = ch;
    for (;;) {
        ch = m_src.get();
        bool flonum = isOneOf(last, "eEpP") && isOneOf(ch, "+-");
        maybe_float = maybe_float || flonum || ch == '.';
        if (!ch.isAlNum() && ch != '.' && !flonum) {
            m_src.unget();
            break;
        }
        ret += ch;
        last = ch;
    }
    return makeToken(maybe_float ? PPFloat : PPNumber, ret);
}

Token* Lexer::getIdentifier(UChar ch) {
    UString ret{ch, 1};
    while(m_src >> ch) {
        switch(ch) {
            case 'a' ... 'z': case 'A' ... 'Z':
            case '0' ... '9': case 0x80000000 ... 0xffffffff:
                ret += ch; break;
            case '\\': if(m_src.want('u')) ret += getUCN(4);
                       else if(m_src.want('U')) ret += getUCN(8);
                       else {
            default:       m_src.unget();
                           goto endloop;
                       }
        }
    }
    endloop:
    return makeToken(Identifier, ret);
}

Token* Lexer::getChar() {
    UString ret{};
    UChar ch;
    while(m_src >> ch) {
        if(ch == '\'') break;
        if(ch == '\\') ret += getEscapedChar();
    }
    return makeToken(Character, ret);
}

Token* Lexer::getString() {
    UString ret{};
    UChar ch;
    while(m_src >> ch) {
        if(ch == '\"') break;
        if(ch == '\\') ret += getEscapedChar();
    }
    return makeToken(String, ret);
}

UChar Lexer::getEscapedChar() {
    switch(auto ch = m_src.get()) {
        case '\'': case '\"': case '\\': case '?':
            return ch;
        case 'a': return '\a';
        case 'b': return '\b';
        case 'f': return '\f';
        case 'n': return '\n';
        case 'r': return '\r';
        case 't': return '\t';
        case 'v': return '\v';
        case 'x': return getHexChar();
        case 'u': return getUCN(4);
        case 'U': return getUCN(8);
        case '0': case '1': case '2': case '3':
        case '4': case '5': case '6': case '7':
            return getOctChar(ch);
        default: derr << sourceLoc() << "unknown escape sequence " << ch;
    }
}

UChar Lexer::getUCN(unsigned len) {
    UChar::ValueType ret = 0;
    for(auto count = 0; count <= len; ++count) {
        if(!m_src.peek().isHex())
            derr << sourceLoc() 
                << "expecting hexadecimal, but is in base " << m_src.get();
        ret <<= 4;
        ret |= m_src.get().toHex();
    }
    return {ret};
}

UChar Lexer::getHexChar() {
    UChar::ValueType ret = 0;
    if(!m_src.peek().isHex())
        derr << sourceLoc()
            << "expecting hexadecimal, but is in base " << m_src.get();
    for(int size = 0; size <= 32; size += 4) {
        if(!m_src.peek().isHex())
            break;
        ret <<= 4;
        ret |= m_src.get().toHex();
    }
    return {ret};
}

// the first is impossible to be non-octonary
UChar Lexer::getOctChar(UChar ch) {
    assert(ch.isOct());
    UChar::ValueType c = ch;
    for(auto count = 1; count <= 3; ++count) {
        if(m_src.peek().isOct()) 
            break;
        c <<= 3;
        c |= m_src.get().toOct();
    }
    return {c};
}
