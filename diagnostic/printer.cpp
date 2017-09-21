#include "text/uchar.hpp"
#include "text/ustring.hpp"
#include "diagnostic/printer.hpp"
#include "diagnostic/sourceloc.hpp"

using namespace Compiler::Text;
using namespace Compiler::Diagnostic;

Printer::Printer(DiagnoseFlag flag) {
    this->operator<<(flag);
}

Printer::~Printer() noexcept(false) {
    if(m_mode == DIAGNOSTIC_ERROR)
        throw 0; // TODO: better exception object
}

Printer& Printer::operator<<(DiagnoseFlag flag) {
    m_mode = flag;
    switch(flag) {
        case DIAGNOSTIC_ERROR:
            fputs("Error: ", stderr); break;
        case DIAGNOSTIC_WARNING:
            fputs("Warning: ", stderr); break;
    }
    return *this;
}

Printer& Printer::operator<<(const SourceLoc *loc) {
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

Printer& Printer::operator<<(const SourceLoc &loc) {
    return operator<<(&loc);
}

Printer& Printer::operator<<(char c) {
    fputc(c, stderr);
    return *this;
}

Printer& Printer::operator<<(const char *str) {
    fprintf(stderr, "%s", str);
    return *this;
}

Printer& Printer::operator<<(unsigned i) {
    fprintf(stderr, "%u", i);
    return *this;
}

Printer& Printer::operator<<(const UChar &uc) {
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

Printer& Printer::operator<<(const UString &us) {
    for(auto &&uc : us) 
        operator<<(uc);
    return *this;
}
