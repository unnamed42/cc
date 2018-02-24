#include "lexical/tokentype.hpp"
#include "semantic/opcode.hpp"

#include <cassert>

namespace impl = Compiler::Semantic;

using namespace Compiler::Lexical;
using namespace Compiler::Semantic;

OpCode impl::toBinaryOpCode(TokenType type) noexcept {
    switch(type) {
        case Star: return OpMul;
        case Div: return OpDiv;
        case Mod: return OpMod;
        case Add: return OpAdd;
        case Sub: return OpSub;
        case LeftShift: return OpLeftShift;
        case RightShift: return OpRightShift;
        case LessThan: return OpLess;
        case GreaterThan: return OpGreater;
        case LessEqual: return OpLessEqual;
        case GreaterEqual: return OpGreaterEqual;
        case Equal: return OpEqual;
        case NotEqual: return OpNotEqual;
        case Ampersand: return OpBitAnd;
        case BitXor: return OpBitXor;
        case BitOr: return OpBitOr;
        case LogicalAnd: return OpAnd;
        case LogicalOr: return OpOr;
        default: assert(false);
    }
}
