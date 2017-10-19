#include "text/uchar.hpp"
#include "text/ustring.hpp"
#include "lexical/token.hpp"
#include "lexical/tokentype.hpp"
#include "semantic/decl.hpp"
#include "semantic/type.hpp"
#include "semantic/qualtype.hpp"
#include "semantic/typeenum.hpp"
#include "diagnostic/logger.hpp"
#include "diagnostic/sourceloc.hpp"

#include <cstdio>

namespace impl = Compiler::Diagnostic;

using namespace Compiler::Text;
using namespace Compiler::Lexical;
using namespace Compiler::Semantic;
using namespace Compiler::Diagnostic;

static void defaultIntegerPrinter(uint32_t i) noexcept {
    fprintf(stderr, "%u", i);
}

template <class Enum>
static void enumPrinter(uint32_t spec) noexcept {
    uint32_t mask = 1;
    bool space = false;
    while(spec) {
        if(spec & mask) {
            spec ^= mask;
            if(space) 
                fputc(' ', stderr);
            fputs(toString(static_cast<Enum>(mask)), stderr);
            space = true;
        }
        mask <<= 1;
    }
}

Logger::IntegerPrinter Logger::specifiers{ &enumPrinter<Specifier> };
Logger::IntegerPrinter Logger::storageClasses{ &enumPrinter<StorageClass> };
Logger::IntegerPrinter Logger::qualifiers{ &enumPrinter<Qualifier> };

Logger::Logger(DiagnoseFlag flag) noexcept : m_printer(&defaultIntegerPrinter) {
    this->operator<<(flag);
}

Logger::~Logger() noexcept(false) {
    fputc('\n', stderr);
    if(m_mode == DIAGNOSTIC_ERROR)
        throw 0; // TODO: better exception object
}

Logger& Logger::operator<<(IntegerPrinter printer) noexcept {
    m_printer = printer;
    return *this;
}

Logger& Logger::operator<<(DiagnoseFlag flag) noexcept {
    m_mode = flag;
    switch(flag) {
        case DIAGNOSTIC_ERROR:
            fputs("error: \n", stderr); break;
        case DIAGNOSTIC_WARNING:
            fputs("warning: \n", stderr); break;
    }
    return *this;
}

Logger& Logger::operator<<(const SourceLoc *loc) noexcept {
    auto beg = loc->lineBegin;
    auto col = loc->column;
    auto len = loc->length;
    
    if(loc->path)
        fprintf(stderr, "In file %s:%u:%u:\n", loc->path, loc->line, col);
    else
        fprintf(stderr, "In temporary text source %u:%u:\n", loc->line, col);
    
    char buffer;
    auto file = loc->file;
    auto here = ftell(file);
    fseek(file, beg, SEEK_SET);
    while(fread(&buffer, sizeof(char), 1, file) == 1) {
        if(buffer == '\n')
            break;
        fputc(buffer, stderr);
    }
    fseek(file, here, SEEK_SET);
    
    fputc('\n', stderr);
    while(col--)
        fputc(' ', stderr);
    while(--len)
        fputc('~', stderr);
    fputc('^', stderr);
    fputc('\n', stderr);
    return *this;
}

Logger& Logger::operator<<(char c) noexcept {
    fputc(c, stderr);
    return *this;
}

Logger& Logger::operator<<(int i) noexcept {
    fprintf(stderr, "%d", i);
    return *this;
}

Logger& Logger::operator<<(const char *str) noexcept {
    fputs(str, stderr);
    return *this;
}

Logger& Logger::operator<<(uint32_t i) noexcept {
    m_printer(i);
    m_printer = &defaultIntegerPrinter;
    return *this;
}

Logger& Logger::operator<<(const UChar &uc) noexcept {
    UChar::ValueType val = uc;
    if(!val)
        return *this;
    
    while(!(val & 0xff000000))
        val <<= 8;
    while(val) {
        fputc(static_cast<char>(val >> 24), stderr);
        val <<= 8;
    }
    return *this;
}

Logger& Logger::operator<<(const UString &us) noexcept {
    for(auto &&uc : us) 
        operator<<(uc);
    return *this;
}

Logger& Logger::operator<<(QualType qt) noexcept {
    operator<<(qt.get());
    if(qt.qual())
        *this << ' ' << qualifiers << qt.qual();
    return *this;
}

Logger& Logger::operator<<(Type *type) noexcept {
    type->print(*this);
    return *this;
}

Logger& Logger::operator<<(Token *tok) noexcept {
    tok->print(*this);
    return *this;
}

Logger& Logger::operator<<(TokenType tokType) noexcept {
    return operator<<(toString(tokType));
}

Logger& Logger::operator<<(Specifier spec) noexcept {
    return operator<<(toString(spec));
}

Logger& Logger::operator<<(Qualifier qual) noexcept {
    return operator<<(toString(qual));
}

Logger& Logger::operator<<(StorageClass stor) noexcept {
    return operator<<(toString(stor));
}

Logger& Logger::at(const SourceLoc *loc) noexcept {
    return operator<<(loc);
}
