#include "utils/mempool.hpp"
#include "semantic/type.hpp"
#include "semantic/typeenum.hpp"
#include "constexpr/value.hpp"

namespace impl = Compiler::ConstExpr;

using namespace Compiler::Text;
using namespace Compiler::Utils;
using namespace Compiler::Semantic;
using namespace Compiler::ConstExpr;

Value::Value(long long l) : m_type(makeNumberType(LLong)), lval(l) {}

Value::Value(double d) : m_type(makeNumberType(Double)), dval(d) {}

Value::Value(const UString *str) : m_type(makePointerType(makeNumberType(Char), Const)), sval(str) {}

const Type* Value::type() const noexcept { return m_type; }

Value* impl::makeNumberValue(long long l) {
    return new (pool) Value(l);
}

Value* impl::makeDoubleValue(double d) {
    return new (pool) Value(d);
}

Value* impl::makeStringValue(const UString *str) {
    return new (pool) Value(str);
}
