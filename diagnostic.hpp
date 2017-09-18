#ifndef DIAGNOSTIC_HPP
#define DIAGNOSTIC_HPP

namespace Compiler {

struct SourceLoc;

void error(const char *format, ...) noexcept(false);
void error(const SourceLoc *loc, const char *format, ...) noexcept(false);

void warning(const char *format, ...) noexcept;
void warning(const SourceLoc *loc, const char *format, ...) noexcept;

}

#endif // DIAGNOSTIC_HPP
