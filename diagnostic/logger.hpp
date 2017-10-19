#ifndef LOGGER_HPP
#define LOGGER_HPP

#include "common.hpp"

namespace Compiler {

namespace Text {
class UChar;
class UString;
}
namespace Lexical {
enum TokenType : uint32_t;
class Token;
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
        explicit Logger(DiagnoseFlag flag = DIAGNOSTIC_WARNING) noexcept;
        ~Logger() noexcept(false);
        
        self& operator<<(IntegerPrinter) noexcept;
        self& operator<<(uint32_t) noexcept;
        
        self& operator<<(char) noexcept;
        self& operator<<(int) noexcept;
        self& operator<<(const char*) noexcept;
        self& operator<<(const SourceLoc*) noexcept;
        self& operator<<(DiagnoseFlag) noexcept;
        
        self& operator<<(const Text::UChar &u) noexcept;
        self& operator<<(const Text::UString &s) noexcept;
        
        self& operator<<(Lexical::Token*) noexcept;
        self& operator<<(Lexical::TokenType) noexcept;
        
        self& operator<<(Semantic::Specifier) noexcept;
        self& operator<<(Semantic::Qualifier) noexcept;
        self& operator<<(Semantic::StorageClass) noexcept;
        
        self& operator<<(Semantic::Type*) noexcept;
        self& operator<<(Semantic::QualType) noexcept;
        
        self& at(const SourceLoc*) noexcept;
        
        template <class HasSource>
        inline self& at(const HasSource *obj) noexcept { return operator<<(obj->sourceLoc()); }
};

} // namespace Diagnostic
} // namespace Compiler

/**< compiler diagnostic error */
#define derr Compiler::Diagnostic::Logger(Compiler::Diagnostic::DIAGNOSTIC_ERROR)
/**< compiler diagnostic warning */
#define dwarn Compiler::Diagnostic::Logger(Compiler::Diagnostic::DIAGNOSTIC_WARNING)

#endif // LOGGER_HPP
