#include "error.hpp"
#include "token.hpp"

#include <cstdio>
#include <cstdarg>

using namespace compiler;

static void vmessage(const file_pos &loc, const char *format, std::va_list args) {
    if(loc.m_name)
        std::fprintf(stderr, "In file %s:%u:%u:\n", loc.m_name, loc.m_line, loc.m_column);
    else
        std::fprintf(stderr, "In temporary string %u:%u:\n", loc.m_line, loc.m_column);
    print_fpos(loc);
    std::vfprintf(stderr, format, args);
    std::putc('\n', stderr);
}

void compiler::print_fpos(const file_pos &p) noexcept {
    // Error/Warning occurs when the unexpected character extracted
    static constexpr unsigned int start = 2;
    
    auto str = p.m_begin;
    for(;*str!='\n' && *str!='\0'; ++str) 
        std::putc(*str, stderr);
    std::putc('\n', stderr);
    for(auto i = start; i < p.m_column; ++i)
        std::putc(' ', stderr);
    std::fprintf(stderr, "^\n");
}

void compiler::error(const char *format, ...) throw(int) {
    std::va_list args;
    va_start(args, format);
    std::vfprintf(stderr, format, args);
    va_end(args);
    
    // TODO: A better way to abort
    throw 0;
}

void compiler::error(const token *tok, const char *format, ...) throw(int) {
    std::va_list args;
    va_start(args, format);
    vmessage(tok->m_pos, format, args);
    va_end(args);
    
    throw 0;
}

void compiler::error(const file_pos &loc, const char *format, ...) throw(int) {
    std::va_list args;
    va_start(args, format);
    vmessage(loc, format, args);
    va_end(args);
    
    // TODO: A better way to abort
    throw 0;
}

void compiler::warning(const char *format, ...) noexcept {
    std::va_list args;
    va_start(args, format);
    std::vfprintf(stderr, format, args);
    va_end(args);
}

void compiler::warning(const file_pos &loc, const char *format, ...) noexcept {
    std::va_list args;
    va_start(args, format);
    vmessage(loc, format, args);
    va_end(args);
}

void compiler::warning(const token *tok, const char *format, ...) noexcept {
    std::va_list args;
    va_start(args, format);
    vmessage(tok->m_pos, format, args);
    va_end(args);
}
