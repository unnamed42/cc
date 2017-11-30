#include "lexer.hpp"

#include <limits>
#include <cctype>
#include <fstream>
#include <unordered_map>

using namespace compiler;

using string = lexer::string;
using char_t = lexer::char_t;
using file_map = std::unordered_map<string, string>;

static file_map files{};

static file_map::iterator read_file(const char*);

static void append(string&, char_t, encoding);
static void append16(string&, char16_t);
static void append32(string&, char32_t);

static int value_of(char_t); // translate hex/oct to its real value

static bool is_oct(char_t);
static bool is_one_of(char_t, const char*);

lexer::lexer():m_text(nullptr), m_pos(nullptr), m_loc() {}

lexer::lexer(const char *location) {
    auto it = read_file(location);
    m_text = &it->second;
    m_loc.m_name = it->first.c_str();
    m_pos = m_loc.m_begin = m_text->c_str();
    m_loc.m_line = m_loc.m_column = 1;
}

lexer::lexer(const string &src)
    :m_text(&src), m_pos(src.c_str()), m_loc() {}

token* lexer::make_token(char_t attr) const {
    return compiler::make_token(attr, m_loc);
}

void lexer::set_line(unsigned int line) {m_loc.m_line = line;}

char_t lexer::getc() {
    auto ch = peekc();
    ++m_pos;
    
    if(ch == '\n' && peekc() != 0) {
        ++m_loc.m_line;
        m_loc.m_begin = m_pos;
        m_loc.m_column = 1;
    } else
        ++m_loc.m_column;
    return ch;
}

// TODO: platform-dependent newline
char_t lexer::peek_helper() {
    if(empty()) return 0;
    
    auto ch = *m_pos;
    
    if(ch == '\\') {
        if(!end()) {
            m_loc.m_begin = (m_pos += 2);
            ++m_loc.m_line;
            m_loc.m_column = 1;
            return peek_helper();
        }
    }
    
    return ch;
}

char_t lexer::peekc() {
    char_t ch = peek_helper();
    
    if((ch & 0x80) == 0) // ASCII, 0xxxxxxx
        return ch;
    else if((ch & 0xe0) == 0xc0) // 110xxxxx 10xxxxxx
        return (ch << 8) | peek_helper();
    else if((ch & 0xe0) == 0xe0) {// 1110xxxx 10xxxxxx 10xxxxxx
        ch = (ch << 8) | peek_helper();
        return (ch << 8) | peek_helper();
    } else if((ch & 0xf8) == 0xf0) { // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        ch = (ch << 8) | peek_helper();
        ch = (ch << 8) | peek_helper();
        return (ch << 8) | peek_helper();
    } else 
        error("Invalid character %c", ch);
    
    switch(ch & 0xf8) {
        case 0xf0:
        case 0xe0:
        default:
            return ch;
    }
    
    return 0;
}

void lexer::ungetc() {
    auto ch = *--m_pos;
    if(ch == '\n') {
        if(m_pos != m_text->c_str() && m_pos[-1] == '/') {
            --m_pos;
            // TODO: line begin, column handler
            --m_loc.m_line;
            ungetc();
        } else
            --m_loc.m_line;
    } else 
        --m_loc.m_column;
}

string lexer::getline() {
    string result{};
    for(int ch;;) {
        ch = getc();
        if(ch == '\n' || ch == '\0') break;
        result += ch;
    }
    return result;
}

void lexer::ignore(char_t ch, bool newline) {
    bool has_newline = false;
    for(int c = getc(); c != '\0'; c = getc()) {
        if(c == '\n') has_newline = true;
        else if(!std::isspace(c)) has_newline = false;
        if(c == ch && (!newline || has_newline)) break;
    }
}

bool lexer::expect(char_t ch) {
    if(ch == peekc()) {
        getc();
        return true;
    }
    return false;
}

token_list lexer::parse_line() {
    token_list result{};
    for(;;) {
        auto tok = get();
        if(tok->m_attr == Newline
        || tok->m_attr == Eof) 
            break;
        result.push_back(tok);
    }
    return result;
}

bool lexer::end() const {
    return m_pos == m_text->c_str() + m_text->length();
}

bool lexer::empty() const {
    return m_text == nullptr;
}

bool lexer::valid() const {
    return !(empty() || end());
}

void lexer::skip_line() {
    for(;;) {
        auto ch = getc();
        if(ch == '\n' || ch == '\0')
            break;
    }
}

void lexer::skip_block_comment() {
    for(;;) {
        auto ch = getc();
        if(ch == '\0') {
            error(m_loc, "Unexpected end-of-file");
            break;
        } else if(ch == '*' && peekc() == '/') {
            // do not use getc() in condition because this
            // will not end comment block when facing "**/"
            getc(); break;
        }
    }
}

bool lexer::skip_space() {
    bool result = false;
    for(;;) {
        switch(getc()) {
            case ' ': case '\f': case '\r':
            case '\t': case '\v': 
                continue;
            case '\n':
                result = true; continue;
            case '/': 
                if(expect('*')) {skip_block_comment(); continue;}
                if(expect('/')) {skip_line(); continue;}
                // intended fall-through
            default:
                ungetc(); return result;
        }
    }
    return result;
}

// TODO: trigraphs
/* 5.2.1.1 Trigraph sequences
 *
 * Before any other processing takes place, each occurrence of one of the following
 * sequences of three characters (called trigraph sequences) is replaced with the
 * corresponding single character.
 * ??= #  ??( [  ??/ \
 * ??) ]  ??' ^  ??< {
 * ??! |  ??> }  ??- ~
 * No other trigraph sequences exist. Each ? that does not begin one of the trigraphs listed
 * above is not changed.
 */
token* lexer::get() {
    // TODO: record the correct position of newline
    if(skip_space()) return make_token(Newline);
    
    auto ch = getc();
    token* temp = nullptr;
    
    if(ch == '\0') return make_token(Eof);
    else if(std::isdigit(ch)) return get_number(ch);
    else if(std::isalpha(ch) || ch == '_' || ch == '$') {
        if(ch == 'L') {
            if(expect('\'')) return get_char(WCHAR);
            if(expect('\"')) return get_string(WCHAR);
        }
        return get_identifier(ch);
    } 
    
    // parsing delimiter with greedy policy
    switch(ch) {
        case '\'': return get_char();
        case '\"': return get_string();
        case '\\':
            if(expect('u')) return get_identifier(get_UCN(4), CHAR16);
            if(expect('U')) return get_identifier(get_UCN(8), CHAR32);
            return make_token(Escape);
        case '=': return make_token(expect('=') ? Equal : Assign);
        case '+': 
            if(expect('+')) return make_token(Inc);
            if(expect('=')) return make_token(AddAssign);
            return make_token(Add);
        case '-':
            if(expect('-')) return make_token(Dec);
            if(expect('=')) return make_token(SubAssign);
            if(expect('>')) return make_token(MemberPtr);
            return make_token(Sub);
        case '*': return make_token(expect('=') ? MulAssign : Star);
        case '/': return make_token(expect('=') ? DivAssign : Div);
        case '%': 
            temp = get_digraph(ch);
            if(temp) return temp;
            if(expect('=')) return make_token(ModAssign);
            return make_token(Mod);
        case '&': 
            if(expect('&')) return make_token(LogicalAnd);
            if(expect('=')) return make_token(BitAndAssign);
            return make_token(Ampersand);
        case '|':
            if(expect('|')) return make_token(LogicalOr);
            if(expect('=')) return make_token(BitOrAssign);
            return make_token(BitOr);
        case '^': return make_token(expect('=') ? BitXorAssign : BitXor);
        case '~': return make_token(BitNot);
        case '!': return make_token(expect('=') ? NotEqual : LogicalNot);
        case '<':
            temp = get_digraph(ch);
            if(temp) return temp;
            if(expect('<')) return make_token(expect('=') ? LeftShiftAssign : LeftShift);
            return make_token(expect('=') ? LessEqual : LessThan);
        case '>':
            if(expect('>')) return make_token(expect('=') ? RightShiftAssign : RightShift);
            return make_token(expect('=') ? GreaterEqual : GreaterThan);
        case ':': 
            temp = get_digraph(ch);
            if(temp) return temp;
            return make_token(Colon);
        case '#': return make_token(expect('#') ? StringConcat : Pound);
        case '(': case ')': case ',': case ';': case '[':
        case ']': case '{': case '}': case '?':
            return make_token(ch);
        case '.':
            if(std::isdigit(peekc())) {ungetc(); return get_number(ch);}
            if(expect('.')) {
                if(expect('.')) 
                    return make_token(Ellipsis);
                ungetc();
            }
            return make_token(Dot);
        default: error(m_loc, "Unrecongnized character %c", ch); return nullptr;
    }
}


/* C99 6.4.6 Punctuators
 * 
 * In all aspects of the language, the six tokens
 * <: :> <% %> %: %:%:
 * behave, respectively, the same as the six tokens
 * [  ]  {  }  #  ##
 * except for their spelling.
 */
// digraphs are recognized during lexical analysis
token* lexer::get_digraph(char_t ch) {
    switch(ch) {
        case '<':
            if(expect(':')) return make_token(LeftSubscript);
            if(expect('%')) return make_token(BlockOpen);
            return nullptr;
        case ':': return expect('>') ? make_token(RightSubscript) : nullptr;
        case '%':
            if(expect('>')) return make_token(BlockClose);
            if(expect(':')) {
                if(expect('%'))  {
                    if(expect(':')) return make_token(StringConcat);
                    ungetc(); // should be '%'
                }
                return make_token(Pound);
            }
            // intended fall-through
        default: return nullptr;
    }
}

// size can only be 4 or 8
char_t lexer::get_UCN(int size) {
    int result = 0;
    for(int count = 0; count <= size; ++count) {
        if(!isxdigit(peekc()))
            error(m_loc, "Expecting hexademical, but get %c", getc());
        result = (result << 4) | value_of(getc());
    }
    return result;
}

char_t lexer::get_hex_char() {
    int result = 0;
    if(!isxdigit(peekc())) 
        error(m_loc, "Expecting hexademical, but get %c", getc());
    for(int size = 0; size <= 32; size += 4) {
        if(!isxdigit(peekc()))
            break;
        result = (result << 4) | value_of(getc());
    }
    return result;
}

// the first is impossible to be non-octonary
char_t lexer::get_oct_char(char_t ch) {
    for(int count = 1; count <= 3; ++count) {
        if(!is_oct(peekc())) break;
        ch = (ch << 3) | value_of(getc());
    }
    return ch;
}

char_t lexer::get_escaped_char(encoding &enc) {
    enc = ASCII;
    switch(int ch = getc()) {
        case '\'': case '\"': case '\\': case '?':
            return ch;
        case 'a': return '\a';
        case 'b': return '\b';
        case 'f': return '\f';
        case 'n': return '\n';
        case 'r': return '\r';
        case 't': return '\t';
        case 'v': return '\v';
        case 'x': enc = CHAR32; return get_hex_char();
        case 'u': enc = CHAR16; return get_UCN(4);
        case 'U': enc = CHAR32; return get_UCN(8);
        case '0': case '1': case '2': case '3':
        case '4': case '5': case '6': case '7':
            enc = CHAR32; return get_oct_char(ch);
        default:
            warning(m_loc, "Unknown escape sequence %c", ch);
            return ch;
    }
}

token* lexer::get_char(encoding enc) {
    string result{};
    auto attr = static_cast<token_attr>(static_cast<unsigned>(Character) | reinterpret_cast<unsigned&>(enc));
    for(;;) {
        auto ch = getc();
        if(ch == '\'') break;
        if(ch == '\\') ch = get_escaped_char(enc);
        append(result, ch, enc);
    }
    return compiler::make_token(attr, m_loc, result);
}

token* lexer::get_string(encoding enc) {
    string result{};
    auto attr = static_cast<token_attr>(static_cast<unsigned>(String) | reinterpret_cast<unsigned&>(enc));
    for(;;) {
        auto ch = getc();
        if(ch == '\"') break;
        else if(ch == '\n') error(m_loc, "Unterminated string literal");
        else if(ch == '\\') ch = get_escaped_char(enc);
        append(result, ch, enc);
    }
    return compiler::make_token(attr, m_loc, result);
}

token* lexer::get_number(char_t ch) {
    bool maybe_float = (ch == '.');
    string result(1, ch);
    auto last = ch;
    for (;;) {
        ch = getc();
        bool flonum = is_one_of(last, "eEpP") && is_one_of(ch, "+-");
        maybe_float = maybe_float || flonum || ch == '.';
        if (!std::isalnum(ch) && ch != '.' && !flonum) {
            ungetc();
            break;
        }
        result += ch;
        last = ch;
    }
    return compiler::make_token(maybe_float ? PPFloat : PPNumber, m_loc, result);
}

token* lexer::get_identifier(char_t ch, encoding enc) {
    string result{};
    append(result, ch, enc);
    for(;;) {
        ch = getc();
        if(std::isalnum(ch) || ch == '_' || ch == '$')
            result += ch;
        else if(ch == '\\') {
            if(expect('u')) append(result, get_UCN(4), CHAR16);
            else if(expect('U')) append(result, get_UCN(8), CHAR32);
            else break;
        } else 
            break;
    }
    ungetc();
    
    auto attr = string_to_attr(result);
    if(attr == Error)
        return compiler::make_token(Identifier, m_loc, result);
    return make_token(attr);
}


file_map::iterator read_file(const char *location) {
    auto it = files.find(location);
    if(it != files.end())
        return it;
    
    std::ifstream file(location, std::ios::in|std::ios::binary|std::ios::ate);
    if(!file.is_open())
        error("%s: Cannot open file or file does not exist\n", location);
    unsigned int file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::string result(file_size + 1, '\0');
    file.read(&result.front(), file_size);
    
    return files.insert({location, std::move(result)}).first;
}

int value_of(char_t ch) {
    if(std::isdigit(ch))
        return ch - '0';
    else if('a' <= ch && ch <= 'f') 
        return ch - 'a' + 10;
    else if('A' <= ch && ch <= 'F') 
        return ch - 'A' + 10;
    else 
        return 0;
}

bool is_oct(char_t ch) {return '0' <= ch && ch <= '7';}

bool is_one_of(char_t ch, const char_t *pattern) {
    for(; *pattern; ++pattern) 
        if(ch == *pattern) return true;
    return false;
}

void append(string &str, char_t ch, encoding enc) {
    switch(enc) {
        case ASCII: str += ch; break;
        case CHAR16: append16(str, ch); break;
        case CHAR32: append32(str, ch); break;
    }
}

void append16(string &str, char_t ch) {
    static constexpr auto uchar_max = std::numeric_limits<unsigned char>::max();
    str += ch & uchar_max;
    str += (ch >>= 8) & uchar_max;
}

void append32(string &str, char_t ch) {
    append16(str, ch & 0xffff);
    append16(str, ch >>= 16);
}
