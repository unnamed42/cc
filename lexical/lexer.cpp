#include "lexical/tokentype.hpp"
#include "sourceloc.hpp"
#include "lexical/token.hpp"
#include "lexical/lexer.hpp"
#include "mempool.hpp"
#include "diagnostic.hpp"

using namespace Compiler::Text;
using namespace Compiler::Lexical;

Lexer::Lexer(const char *path)
    : m_src(readFile(path), path) {}

Lexer::Lexer(const String &src)
    : m_src(src.c_str(), nullptr) {}

Token* Lexer::makeToken(TokenType type) {
    // TODO: location fix
    auto loc = makeSourceLoc(&m_src.sourceLoc());
    loc->length = strlen(toString(type));
    loc->column -= loc->length;
    return impl::makeToken(loc, type);
}

void Lexer::logPos() {
    m_col = m_src.column();
}

const SourceLoc* Lexer::sourceLoc() const noexcept {
    return &m_src.sourceLoc();
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
            if(m_src.expect('\'')) return getChar();
            if(m_src.expect('\"')) return getString();
        case 'a' ... 'z': case 'A' ... 'K': case 'M' ... 'Z':
        case '_': case '$': case 0x80000000 ... 0xffffffff:
            return getIdentifier(ch);
        case '\'': return getChar();
        case '\"': return getString();
        case '\\':
            if(m_src.expect('u')) return getIdentifier(getUCN(4));
            if(m_src.expect('U')) return getIdentifier(getUCN(8));
            return makeToken(Escape);
        case '=': return makeToken(m_src.expect('=') ? Equal : Assign);
        case '+':
            if(m_src.expect('+')) return makeToken(Inc);
            if(m_src.expect('=')) return makeToken(AddAssign);
            return makeToken(Add);
        case '-':
            if(m_src.expect('-')) return makeToken(Dec);
            if(m_src.expect('=')) return makeToken(SubAssign);
            if(m_src.expect('>')) return makeToken(MemberPtr);
            return makeToken(Sub);
        case '*': return makeToken(m_src.expect('=') ? MulAssign : Star);
        case '/': return makeToken(m_src.expect('=') ? DivAssign : Div);
        case '%':
            temp = getDigraph(ch);
            if(temp) return temp;
            if(m_src.expect('=')) return makeToken(ModAssign);
            return makeToken(Mod);
        case '&':
            if(m_src.expect('&')) return makeToken(LogicalAnd);
            if(m_src.expect('=')) return makeToken(BitAndAssign);
            return makeToken(Ampersand);
        case '|':
            if(m_src.expect('|')) return makeToken(LogicalOr);
            if(m_src.expect('=')) return makeToken(BitOrAssign);
            return makeToken(BitOr);
        case '^': return makeToken(m_src.expect('=') ? BitXorAssign : BitXor);
        case '~': return makeToken(BitNot);
        case '!': return makeToken(m_src.expect('=') ? NotEqual : LogicalNot);
        case '<':
            temp = getDigraph(ch);
            if(temp) return temp;
            if(m_src.expect('<')) return makeToken(m_src.expect('=') ? LeftShiftAssign : LeftShift);
            return makeToken(m_src.expect('=') ? LessEqual : LessThan);
        case '>':
            if(m_src.expect('>')) return makeToken(m_src.expect('=') ? RightShiftAssign : RightShift);
            return makeToken(m_src.expect('=') ? GreaterEqual : GreaterThan);
        case ':':
            temp = getDigraph(ch);
            if(temp) return temp;
            return makeToken(Colon);
        case '#': return makeToken(m_src.expect('#') ? StringConcat : Pound);
        case '(': case ')': case ',': case ';': case '[':
        case ']': case '{': case '}': case '?':
            return makeToken(toTokenType(ch));
        case '.':
            if(isdigit(m_src.peek())) return getNumber(ch);
            if(m_src.expect('.')) {
                if(m_src.expect('.'))
                    return makeToken(Ellipsis);
                m_src.unget();
            }
            return makeToken(Dot);
        default: error(sourceLoc(), "Unrecongnized character %c", ch); 
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
            if(m_src.expect(':')) return makeToken(LeftSubscript);
            if(m_src.expect('%')) return makeToken(BlockOpen);
            return nullptr;
        case ':': return m_src.expect('>') ? makeToken(RightSubscript) : nullptr;
        case '%':
            if(m_src.expect('>')) return makeToken(BlockClose);
            if(m_src.expect(':')) {
                if(m_src.expect('%'))  {
                    if(m_src.expect(':')) return makeToken(StringConcat);
                    m_src.unget();
                }
                return makeToken(Pound);
            }
        default: return nullptr;
    }
}

Token* Lexer::getNumber(UChar ch) {
    bool maybe_float = (ch == '.');
    String ret{};
    append(ret, ch);
    auto last = ch;
    for (;;) {
        ch = m_src.get();
        bool flonum = is_one_of(last, "eEpP") && is_one_of(ch, "+-");
        maybe_float = maybe_float || flonum || ch == '.';
        if (!isalnum(ch) && ch != '.' && !flonum) {
            m_src.unget(ch);
            break;
        }
        append(ret, ch);
        last = ch;
    }
    return makeToken(maybe_float ? PPFloat : PPNumber, ret);
}

Token* Lexer::getIdentifier(UChar ch) {
    string ret{};
    append(ret, ch);
    while(ch = m_src.get()) {
        switch(ch) {
            case 'a' ... 'z': case 'A' ... 'Z':
            case '0' ... '9': case 0x80000000 ... 0xffffffff:
                append(ret, ch); break;
            case '\\': if(m_src.expect('u')) append(ret, getUCN(4));
                       else if(m_src.expect('U')) append(ret, getUCN(8));
                       else {
            default:       m_src.unget(ch);
                           goto endloop;
                       }
        }
    }
    endloop:
    return makeToken(Identifier, ret);
}

Token* Lexer::getChar() {
    String ret{};
    auto type = static_cast<uint32_t>(Character) | reinterpret_cast<uint32_t&>(enc);
    UChar ch;
    while(ch = m_src.get()) {
        if(ch == '\'') break;
        if(ch == '\\') append(ret, getEscapedChar());
    }
    return makeToken(type, ret);
}

Token* Lexer::getString() {
    String ret{};
    auto type = static_cast<uint32_t>(Character) | reinterpret_cast<uint32_t&>(enc);
    UChar ch;
    while(ch = m_src.get()) {
        if(ch == '\"') break;
        if(ch == '\\') append(ret, getEscapedChar());
    }
    return makeToken(type, ret);
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
        case 'x': return get_hex_char();
        case 'u': return getUCN(4);
        case 'U': return getUCN(8);
        case '0': case '1': case '2': case '3':
        case '4': case '5': case '6': case '7':
            return get_oct_char(ch);
        default:
            message(m_src, "Unknown escape sequence %c", ch);
            return ch;
    }
}

UChar Lexer::getUCN(unsigned len) {
    UChar ret = 0;
    for(auto count = 0; count <= len; ++count) {
        if(!isxdigit(m_src.peek()))
            error(m_src, "Expecting hexadecimal, but base %c", m_src.get());
        ret = (ret << 4) | xvalue(m_src.get());
    }
    return ret;
}

UChar Lexer::get_hex_char() {
    UChar ret = 0;
    if(!isxdigit(m_src.peek()))
        error(m_src, "Expecting hexadecimal, but base %c", m_src.get());
    for(int size = 0; size <= 32; size += 4) {
        if(!isxdigit(m_src.peek()))
            break;
        ret = (ret << 4) | xvalue(m_src.get());
    }
    return ret;
}

// the first is impossible to be non-octonary
UChar Lexer::get_oct_char(UChar ch) {
    for(auto count = 1; count <= 3; ++count) {
        if(!is_oct(m_src.peek())) break;
        ch = (ch << 3) | xvalue(m_src.get());
    }
    return ch;
}
