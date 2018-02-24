#ifndef VALUE_HPP
#define VALUE_HPP

namespace Compiler {

namespace Text {
class UString;
}
namespace Semantic {
class Type;
}

namespace ConstExpr {

class Value {
    private:
        Semantic::Type *m_type;
        union {
            long long lval;
            double dval;
            const Text::UString *sval;
        };
    public:
        Value(long long);
        Value(double);
        Value(const Text::UString*);
        
        const Semantic::Type* type() const noexcept;
};

Value* makeNumberValue(long long);
Value* makeDoubleValue(double);
Value* makeStringValue(const Text::UString*);

} // namespace ConstExpr
} // namespace Compiler

#endif // VALUE_HPP
