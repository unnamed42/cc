#include "text/file.hpp"
#include "text/uchar.hpp"
#include "text/buffer.hpp"

#include <cassert>

using namespace Compiler::Text;

File::File(const char *path) {
    open(path);
}

File::File(FILE *f) : m_file(f) {}

File::~File() {
    fclose(m_file);
}

char File::read() {
    char buffer;
    if(fread(&buffer, sizeof(buffer), 1, m_file) == 1)
        return buffer;
    return 0;
}

char File::peekASCII() {
    auto byte = read();
    if(byte)
        ungetASCII();
    return byte;
}

UChar File::get() {
    return UChar{this};
}

UChar File::peek() {
    UChar ret{this};
    unget(ret);
    return ret;
}

void File::ungetASCII() {
    seek(-1);
}

int File::ungetUntilASCII(char byte) {
    int bytes = 0;
    while(seek(-1)) {
        ++bytes;
        if(peekASCII() == byte) {
            ignoreASCII();
            break;
        }
    }
    return bytes;
}

UChar File::unget() {
    UChar::ValueType value = 0;
    int shift = 0;
    #define SHIFT(val, ch, shift) val |= (static_cast<int>(ch) << shift); shift += 8
    if(!seek(-1))
        return 0;
    char ch_ = peekASCII();
    auto utf8Bytes = validUTF8Head(ch_);
    assert(utf8Bytes != 0);
    SHIFT(value, ch_ , shift);
    while(--utf8Bytes) {
        auto success = seek(-1);
        assert(success);
        ch_ = peekASCII();
        assert(validUTF8Byte(ch_));
        SHIFT(value, ch_ , shift);
    }
    #undef SHIFT
    return {value};
}

int File::unget(UChar ch) {
    auto bytes = ch.bytes();
    seek(-bytes);
    return bytes;
}

int File::ungetUntil(UChar ch) {
    if(ch.isASCII())
        ungetUntilASCII(ch & 0xff);
    auto value = UChar::makeInvalid();
    int bytes = 0; // return value
    while(good() && value != ch) {
        value = unget();
        bytes += value.bytes();
    }
    if(bytes)
        seek(value.bytes());
    return bytes;
}

void File::ignoreASCII() {
    seek(1);
}

int File::ignoreUntilASCII(char byte) {
    int bytes = 0;
    while(read() != byte)
        ++bytes;
    return bytes;
}

void File::ignore() {
    get();
}

void File::ignore(UChar ch) {
    seek(ch.bytes());
}

int File::ignoreUntil(UChar ch) {
    int ret = 0;
    while(good()) {
        auto ch_ = get();
        if(ch_ != ch) {
            unget(ch_);
            break;
        } else
            ret += ch_.bytes();
    }
    return ret;
}

void File::open(const char *path) {
    m_file = fopen(path, "rb");
}

void File::flush() {
    fflush(m_file);
}

void File::close() {
    fclose(m_file);
}

long File::tell() {
    return ftell(m_file);
}

bool File::seek(long offset, int origin) {
    return !fseek(m_file, offset, origin);
}

bool File::eof() {
    return feof(m_file);
}

bool File::error() {
    return ferror(m_file);
}

bool File::good() {
    return !(eof() || error());
}

FILE* File::getFILE() {
    return m_file;
}


File::operator bool() {
    return !error();
}

File& File::operator>>(UChar &ch) {
    new (&ch) UChar{this};
    return *this;
}

File& File::operator>>(Buffer &buf) {
    UChar temp;
    while(operator>>(temp))
        buf.append(temp);
    return *this;
}
