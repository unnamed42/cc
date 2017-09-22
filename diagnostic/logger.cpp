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

namespace impl = Compiler::Diagnostic;

using namespace Compiler::Text;
using namespace Compiler::Lexical;
using namespace Compiler::Semantic;
using namespace Compiler::Diagnostic;

static void defaultIntegerPrinter(uint32_t i) {
    fprintf(stderr, "%u", i);
}

template <class Enum>
static void enumPrinter(uint32_t spec) {
    uint32_t mask = 1;
    bool space = false;
    while(spec) {
        if(spec & mask) {
            spec ^= mask;
            if(space) 
                fputc(' ', stderr);
            fprintf(stderr, "%s", toString(static_cast<Enum>(mask)));
            space = true;
        }
        mask <<= 1;
    }
}

Logger::IntegerPrinter Logger::specifiers{ &enumPrinter<Specifier> };
Logger::IntegerPrinter Logger::storageClasses{ &enumPrinter<StorageClass> };
Logger::IntegerPrinter Logger::qualifiers{ &enumPrinter<Qualifier> };

Logger::Logger(DiagnoseFlag flag) : m_printer(&defaultIntegerPrinter) {
    this->operator<<(flag);
}

Logger::~Logger() noexcept(false) {
    if(m_mode == DIAGNOSTIC_ERROR)
        throw 0; // TODO: better exception object
}

Logger& Logger::operator<<(IntegerPrinter printer) noexcept {
    m_printer = printer;
    return *this;
}

Logger& Logger::operator<<(DiagnoseFlag flag) {
    m_mode = flag;
    switch(flag) {
        case DIAGNOSTIC_ERROR:
            fputs("error: \n", stderr); break;
        case DIAGNOSTIC_WARNING:
            fputs("warning: \n", stderr); break;
    }
    return *this;
}

Logger& Logger::operator<<(const SourceLoc *loc) {
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

Logger& Logger::operator<<(const SourceLoc &loc) {
    return operator<<(&loc);
}

Logger& Logger::operator<<(char c) {
    fputc(c, stderr);
    return *this;
}

Logger& Logger::operator<<(int i) {
    fprintf(stderr, "%d", i);
    return *this;
}

Logger& Logger::operator<<(const char *str) {
    fprintf(stderr, "%s", str);
    return *this;
}

Logger& Logger::operator<<(uint32_t i) {
    m_printer(i);
    m_printer = &defaultIntegerPrinter;
    return *this;
}

Logger& Logger::operator<<(const UChar &uc) {
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

Logger& Logger::operator<<(const UString &us) {
    for(auto &&uc : us) 
        operator<<(uc);
    return *this;
}

Logger& Logger::operator<<(QualType qt) {
    return *this << qt.get() << ' ' << qualifiers << qt.qual();
}

Logger& Logger::operator<<(Type *type) {
    type->print(*this);
    return *this;
}

Logger& Logger::operator<<(Token *tok) {
    tok->print(*this);
    return *this;
}

Logger& Logger::operator<<(TokenType tokType) {
    return *this << toString(tokType);
}

Logger& Logger::operator<<(Specifier spec) {
    return *this << toString(spec);
}

Logger& Logger::operator<<(Qualifier qual) {
    return *this << toString(qual);
}

Logger& Logger::operator<<(StorageClass stor) {
    return *this << toString(stor);
}
