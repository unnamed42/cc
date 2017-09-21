#ifndef VALUE_HPP
#define VALUE_HPP

#include "text/uchar.hpp"
#include "text/ustring.hpp"

namespace Compiler {

namespace Diagnostic {
struct SourceLoc;
}

namespace ConstExpr {

class Value {
    protected:
        Diagnostic::SourceLoc *source_;
    public:
        explicit Value(Diagnostic::SourceLoc*) noexcept;
        virtual ~Value() = default;
        
        Diagnostic::SourceLoc* source() noexcept;
        
        virtual bool isInteger() const noexcept;
        virtual bool isDouble()  const noexcept;
        virtual bool isChar()    const noexcept;
        virtual bool isString()  const noexcept;
        
        virtual long*          asInteger() noexcept;
        virtual double*        asDouble()  noexcept;
        virtual Text::UChar*   asChar()    noexcept;
        virtual Text::UString* asString()  noexcept;
        
        virtual Value& operator+=(Value &other) = 0;
};

class IntegerValue : public Value {
    public:
        using ValueType = long;
    private:
        ValueType value;
    public:
        IntegerValue(Diagnostic::SourceLoc *source, ValueType value) noexcept;
        
        bool isInteger() const noexcept override;
        ValueType* asInteger() noexcept override;
        IntegerValue& operator+=(Value &other) override;
};

class DoubleValue : public Value {
    public:
        using ValueType = double;
    private:
        ValueType value;
    public:
        DoubleValue(Diagnostic::SourceLoc *source, ValueType value) noexcept;
        
        bool isDouble() const noexcept override;
        ValueType* asDouble() noexcept override;
        DoubleValue& operator+=(Value &other) override;
};

class CharValue : public Value {
    public:
        using ValueType = Text::UChar;
    private:
        ValueType value;
    public:
        CharValue(Diagnostic::SourceLoc *source, ValueType value) noexcept;
        
        bool isChar() const noexcept override;
        ValueType* asChar() noexcept override;
        CharValue& operator+=(Value &other) override;
};

class StringValue : public Value {
    public:
        using ValueType = Text::UString;
    private:
        ValueType value;
    public:
        StringValue(Diagnostic::SourceLoc *source, ValueType value) noexcept;
        
        bool isString() const noexcept override;
        ValueType* asString() noexcept override;
        StringValue& operator+=(Value &other) override;
};

IntegerValue* makeValue(Diagnostic::SourceLoc *source, long value);
DoubleValue*  makeValue(Diagnostic::SourceLoc *source, double value);
CharValue*    makeValue(Diagnostic::SourceLoc *source, Text::UChar value);
StringValue*  makeValue(Diagnostic::SourceLoc *source, Text::UString value);

} // namespace ConstExpr
} // namespace Compiler

#endif // VALUE_HPP
