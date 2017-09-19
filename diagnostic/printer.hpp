#ifndef PRINTER_HPP
#define PRINTER_HPP

#include "common.hpp"

#include <cstdio>

namespace Compiler {
    
namespace Text {
class UChar;
class UString;
}

namespace Diagnostic {

struct SourceLoc;
    
enum DiagnoseFlag {
    DIAGNOSTIC_WARNING,
    DIAGNOSTIC_ERROR
};
    
class Printer {
    NO_COPY_MOVE(Printer);
    private:
        using self = Printer;
    private:
        DiagnoseFlag m_mode;
    public:
        explicit Printer(DiagnoseFlag flag = DIAGNOSTIC_WARNING) noexcept;
        ~Printer() noexcept(false);
        
        self& operator<<(const char *);
        self& operator<<(const SourceLoc *);
        self& operator<<(const SourceLoc &);
        self& operator<<(DiagnoseFlag);
        self& operator<<(unsigned i);
        self& operator<<(const Text::UChar &u);
        self& operator<<(const Text::UString &s);
};

} // namespace Diagnostic
} // namespace Compiler

#endif // PRINTER_HPP
