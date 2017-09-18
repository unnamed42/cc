#include "text/stream.hpp"
#include "text/ustring.hpp"
#include "diagnostic.hpp"

using namespace Compiler;
using namespace Compiler::Text;

static const UChar invalid = 0xffffffff;

Stream::Stream(const char *path)
    : m_file(path), m_loc(path, m_file.getFILE()), m_peek(invalid) {}

void Stream::newline() noexcept {
    ++m_loc.line;
    m_loc.column = 1;
    m_loc.lineBegin = m_file.tell();
}

UChar Stream::fileGet() {
    auto ch = m_file.get();
    ++m_loc.column;
    if(ch == '\n') {
        ++m_loc.line;
        m_loc.lineBegin = m_file.tell();
        m_loc.column = 1;
    }
    return ch;
}

void Stream::fileUnget(UChar ch) {
    while(ch == '\n') {
        
    }
    while(m_curr >= m_src && *m_curr == '\n') {
        auto p = m_curr;
        while(p >= m_src && *p != '\n')
            --p;
        m_loc.column = m_curr - p;
        m_loc.lineBegin = p;
        --m_loc.line;
    }
    // UTF8
    while((*m_curr & 0xc0) == 0x80) {
        --m_curr;
        --m_loc.column;
    } 
    --m_curr;
    --m_loc.column;
}

UChar Stream::get() noexcept {
    auto ch = peek();
    m_peek = invalid;
    if(ch == '\n')
        newline();
    return ch;
}

UChar Stream::peek() {
    if(m_peek != invalid)
        return m_peek;
    if(!m_file)
        return 0;
    
    UChar ch = m_file.get();
    auto chp = m_file.peek();
    
    if(ch == '\\' && chp == '\n') {
        m_file.ignore();
        m_peek = invalid;
        newline();
        return peek();
    } else if(ch == '?' && chp == '?') {
        m_file.ignore(); // the second '?'
        ch = m_file.get();
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
        switch(m_file.get()) {
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
                m_file.unget(); // current character and the second '?'
        }
        return m_peek = ch;
    }
    
    return m_peek = ch;
}

bool Stream::expect(UChar ch) noexcept {
    if(peek() == ch) {
        get();
        return true;
    }
    return false;
}

void Stream::unget() noexcept {
    bufUnget();
    m_peek = invalid;
}

void Stream::ignore(UChar ch) noexcept {
    bufIgnore(ch);
}

UString Stream::getline() {
    UString result{};
    char ch;
    while(ch = bufGet()) {
        if(ch == '\n')
            break;
        result += ch;
    }
    return result;
}

const char* Stream::position() const noexcept {
    return m_curr;
}

const SourceLoc& Stream::sourceLoc() const noexcept {
    return m_loc;
}

const char* Stream::path() const noexcept {
    return m_loc.path;
}

void Stream::skipLine() noexcept {
    bufIgnore('\n');
}

void Stream::skipBlockComment() noexcept {
    while(bufGood()) {
        bufIgnore('*');
        if(expect('/'))
            break;
    }
}

int Stream::skipSpace() noexcept {
    int ret = 0;
    for(UChar ch;;) {
        switch(ch = get()) {
            case '/': if(expect('*')) {
                          skipBlockComment(); ret |= 1;
                      } else if(expect('/')) {
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

unsigned Stream::line() const noexcept {
    return m_loc.line;
}

unsigned Stream::column() const noexcept {
    return m_loc.column;
}

const char* Stream::lineBegin() const noexcept {
    return m_loc.lineBegin;
}

