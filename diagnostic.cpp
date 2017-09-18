#include "sourceloc.hpp"
#include "diagnostic.hpp"

#include <cstdio>
#include <cstdarg>

namespace impl = Compiler;

static void sourceLocPrinter(const impl::SourceLoc *loc) {
    auto beg = loc->lineBegin;
    auto col = loc->column;
    auto len = loc->length;
    
    if(loc->path)
        fprintf(stderr, "In file %s:%u:%u:\n", loc->path, loc->line, col);
    else
        fprintf(stderr, "In temporary text source %u:%u:\n", loc->line, col);
    
    while(*beg && *beg != '\n')
        fputc(*beg, stderr);
    fputc('\n', stderr);
    while(col--)
        fputc(' ', stderr);
    while(--len)
        fputc('~', stderr);
    fputc('^', stderr);
    fputc('\n', stderr);
}

void impl::error(const char *format, ...) noexcept(false) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    
    throw 0; // TODO: better error signaling
}

void impl::error(const SourceLoc *loc, const char *format, ...) noexcept(false) {
    sourceLocPrinter(loc);
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    
    throw 0; // TODO: better error signaling
}

void impl::warning(const char *format, ...) noexcept {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}

void impl::warning(const SourceLoc *loc, const char *format, ...) noexcept {
    sourceLocPrinter(loc);
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}
