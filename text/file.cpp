#include "text/file.hpp"
#include "text/uchar.hpp"

using namespace Compiler::Text;

File::File(const char *path) {
    open(path);
}

File::File(FILE *f) : m_file(f) {}

File::~File() {
    fclose(m_file);
}

uint8_t File::read() {
    uint8_t buffer;
    fread(&buffer, sizeof(char), 1, m_file);
    return buffer;
}

UChar File::get() {
    return UChar{this};
}

UChar File::peek() {
    UChar ret{this};
    unget(ret);
    return ret;
}

int File::unget() {
    seek(-1, SEEK_CUR);
    return 1;
}

int File::unget(UChar ch) {
    auto bytes = ch.bytes();
    seek(-bytes, SEEK_CUR);
    return bytes;
}

void File::ignore() {
    get();
}

int File::ignore(UChar ch) {
    int ret = 0;
    while(*this) {
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

void File::seek(long offset, int origin) {
    fseek(m_file, offset, origin);
}

bool File::eof() {
    return feof(m_file);
}

bool File::error() {
    return ferror(m_file);
}

File::operator bool() {
    return !(eof() || error());
}
