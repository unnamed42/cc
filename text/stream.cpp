#include "text/uchar.hpp"
#include "text/stream.hpp"
#include "text/ustring.hpp"

using namespace Compiler;
using namespace Compiler::Text;
using namespace Compiler::Diagnostic;

using PosType = Stream::PosType;

Stream::Stream(const char *path)
    : m_file(path), m_loc(path, m_file.getFILE()) {}

void Stream::newline() noexcept {
    ++m_loc.line;
    m_loc.column = 1;
    m_loc.lineBegin = m_file.tell();
}

UChar Stream::get() noexcept {
    UChar ch = peek();
    m_file.ignore(ch);
    ++m_loc.column;
    if(ch.isNewline()) 
        newline();
    return ch;
}

UChar Stream::peek() {
    if(!m_file)
        return UChar::makeInvalid();
    
    auto ch = m_file.get();
    auto chp = m_file.peek();
    
    if(ch == '\\' && chp == '\n') {
        m_file.ignoreASCII();
        newline();
        return peek();
    } else if(ch == '?' && chp == '?') {
        m_file.ignoreASCII(); // the second '?'
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
        switch(chp = m_file.get()) {
            case '(': ch = '['; break;
            case ')': ch = ']'; break;
            case '/': ch = '\\'; break;
            case '\'': ch = '^'; break;
            case '<': ch = '{'; break;
            case '>': ch = '}'; break;
            case '!': ch = '|'; break;
            case '-': ch = '~'; break;
            case '=': ch = '#'; break;
            default:
                m_file.unget(chp); // current character 
                m_file.ungetASCII(); // the second '?'
        }
    }
    
    m_file.ungetUntilASCII(ch);
    return ch;
}

bool Stream::want(UChar ch) noexcept {
    if(peek() == ch) {
        get();
        return true;
    }
    return false;
}

void Stream::unget() noexcept {
    auto ch = m_file.unget();
    --m_loc.column;
    if(ch.isNewline()) {
        auto here = m_file.tell();
        m_file.ungetUntilASCII('\n');
        m_loc.lineBegin = m_file.tell();
        --m_loc.line;
        m_loc.column = 1;
        m_file.seek(here, SEEK_SET);
    }
}

void Stream::ignore(UChar ch) noexcept {
    UChar c;
    while(m_file >> c) {
        if(c.isNewline())
            newline();
        else 
            ++m_loc.column;
        if(c == ch) 
            break;
    }
}

// UString Stream::getline() {
//     UString result{};
//     UChar c;
//     while(!(c = get()).invalid())
//         result += c;
//     return result;
// }

void Stream::skipLine() noexcept {
    ignore('\n');
}

void Stream::skipBlockComment() noexcept {
    while(m_file) {
        ignore('*');
        if(want('/')) 
            return;
    }
}

int Stream::skipSpace() noexcept {
    int ret = 0;
    for(UChar ch;;) {
        switch(ch = get()) {
            case '/': if(want('*')) {
                          skipBlockComment(); ret |= 1;
                      } else if(want('/')) {
                          skipLine(); ret |= 2;
                      } else {
            default:      unget();
            case '\0':    return ret;
                      }
                      continue;
            case ' ': case '\f':
            case '\r': case '\t':
            case '\v':
                ret |= 1; continue;
            case '\n':
                ret |= 2;
        }
    }
}

PosType Stream::pos() {
    return m_file.tell();
}

const SourceLoc& Stream::sourceLoc() const noexcept {
    return m_loc;
}

const char* Stream::path() const noexcept {
    return m_loc.path;
}

unsigned Stream::line() const noexcept {
    return m_loc.line;
}

unsigned Stream::column() const noexcept {
    return m_loc.column;
}

PosType Stream::lineBegin() const noexcept {
    return m_loc.lineBegin;
}

Stream::operator bool() {
    return static_cast<bool>(m_file);
}

Stream& Stream::operator>>(UChar &result) {
    result = get();
    return *this;
}
