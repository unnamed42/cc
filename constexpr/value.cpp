#include "utils/mempool.hpp"
#include "constexpr/value.hpp"
#include "diagnostic/logger.hpp"

namespace impl = Compiler::ConstExpr;

using namespace Compiler::Text;
using namespace Compiler::Utils;
using namespace Compiler::ConstExpr;
using namespace Compiler::Diagnostic;

Value::Value(SourceLoc *loc) noexcept : source_(loc) {}
SourceLoc* Value::source() noexcept { return source_; }
bool Value::isInteger() const noexcept { return false; }
bool Value::isDouble()  const noexcept { return false; }
bool Value::isChar()    const noexcept { return false; }
bool Value::isString()  const noexcept { return false; }
long* Value::asInteger()   noexcept { return nullptr; }
double* Value::asDouble()  noexcept { return nullptr; }
UChar* Value::asChar()     noexcept { return nullptr; }
UString* Value::asString() noexcept { return nullptr; }

#define IMPLEMENT(name) \
    name##Value* impl::makeValue(SourceLoc *source, name##Value::ValueType value) { \
        return new (pool) name##Value{source, move(value)}; \
    } \
    name##Value::name##Value(SourceLoc *source, name##Value::ValueType value) noexcept : Value(source), value(move(value)) {} \
    bool name##Value::is##name() const noexcept { return true; } \
    name##Value::ValueType* name##Value::as##name() noexcept { return &value; } \
    name##Value& name##Value::operator+=(Value &other) 

IMPLEMENT(Integer) {
    if(other.isInteger())
        value += *other.asInteger();
    else if(other.isDouble())
        value += *other.asDouble();
    else if(other.isChar())
        value += *other.asChar();
    else
        derr << source() << "cannot add with expression\n" << other.source();
    return *this;
}

IMPLEMENT(Double) {
    if(other.isInteger())
        value += *other.asInteger();
    else if(other.isDouble())
        value += *other.asDouble();
    else if(other.isChar())
        value += *other.asChar();
    else
        derr << source() << "cannot add with expression\n" << other.source();
    return *this;
}

IMPLEMENT(Char) {
    if(other.isInteger())
        value = value + *other.asInteger();
    else if(other.isChar())
        value = value + *other.asChar();
    else
        derr << source() << "cannot add with expression\n" << other.source();
    return *this;
}

IMPLEMENT(String) {
    if(other.isString())
        value += *other.asString();
    else if(other.isChar())
        value += *other.asChar();
    else
        derr << source() << "cannot add with expression\n" << other.source();
    return *this;
}
