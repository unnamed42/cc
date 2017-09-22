#ifndef LOGGER_HPP
#define LOGGER_HPP

#include "common.hpp"

#include <cstdio>

namespace Compiler {

namespace Text {
class UChar;
class UString;
}
namespace Lexical {
enum TokenType : uint32_t;
}
namespace Semantic {
class Decl;
class Type;
class QualType;
enum Specifier : uint32_t;
enum StorageClass : uint32_t;
enum Qualifier : uint32_t;
}

namespace Diagnostic {

struct SourceLoc;

enum DiagnoseFlag {
    DIAGNOSTIC_WARNING,
    DIAGNOSTIC_ERROR
};

class Logger {
    NO_COPY_MOVE(Logger);
    private:
        struct IntegerPrinter {
            void (*printer)(uint32_t);
            
            IntegerPrinter(void(*p)(uint32_t)) noexcept : printer(p) {}
            void operator()(uint32_t i) const { printer(i); }
        };
    public:
        static IntegerPrinter specifiers;
        static IntegerPrinter storageClasses;
        static IntegerPrinter qualifiers;
    private:
        using self = Logger;
    private:
        DiagnoseFlag   m_mode;
        IntegerPrinter m_printer;
    public:
        explicit Logger(DiagnoseFlag flag = DIAGNOSTIC_WARNING);
        ~Logger() noexcept(false);
        
        self& operator<<(IntegerPrinter) noexcept;
        self& operator<<(uint32_t);
        
        self& operator<<(char);
        self& operator<<(int);
        self& operator<<(const char*);
        self& operator<<(const SourceLoc*);
        self& operator<<(const SourceLoc&);
        self& operator<<(DiagnoseFlag);
        
        self& operator<<(const Text::UChar &u);
        self& operator<<(const Text::UString &s);
        
        self& operator<<(Lexical::TokenType);
        
        self& operator<<(Semantic::Specifier);
        self& operator<<(Semantic::Qualifier);
        self& operator<<(Semantic::StorageClass);
        
        self& operator<<(Semantic::Type*);
        self& operator<<(Semantic::QualType);
};

} // namespace Diagnostic
} // namespace Compiler

/**< compiler diagnostic error */
#define derr Compiler::Diagnostic::Logger(Compiler::Diagnostic::DIAGNOSTIC_ERROR)
/**< compiler diagnostic warning */
#define dwarn Compiler::Diagnostic::Logger(Compiler::Diagnostic::DIAGNOSTIC_WARNING)

#endif // LOGGER_HPP
