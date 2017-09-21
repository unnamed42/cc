#ifndef OPCODE_HPP
#define OPCODE_HPP

#include <cstdint>

namespace Compiler {

namespace Lexical {
enum TokenType : uint32_t;
}

namespace Semantic {

enum OpCode : uint32_t {
    OpMember,           // .
    OpMemberPtr,        // ->
    OpObjectOf,         // unary *
    OpAddressOf,        // unary &
    OpSubscript,        // []
    OpComma,            // ,
    OpNegate,           // unary -
    OpValueOf,          // unary +
    OpAdd,              // binary +
    OpSub,              // binary -
    OpMul,              // binary *
    OpDiv,              // /
    OpMod,              // %
    OpBitAnd,           // binary &
    OpBitOr,            // |
    OpBitXor,           // ^
    OpBitNot,           // ~
    OpLeftShift,        // <<
    OpRightShift,       // >>
    OpLess,             // <
    OpLessEqual,        // <=
    OpGreater,          // >
    OpGreaterEqual,     // >=
    OpEqual,            // ==
    OpNotEqual,         // !=
    OpAnd,              // &&
    OpOr,               // ||
    OpNot,              // !
    OpPrefixInc,        // ++
    OpPostfixInc,
    OpPrefixDec,        // --
    OpPostfixDec,
    OpAssign,           // =
    OpAddAssign,        // +=
    OpSubAssign,        // -=
    OpMulAssign,        // *=
    OpDivAssign,        // /=
    OpModAssign,        // %=
    OpBitAndAssign,     // &=
    OpBitOrAssign,      // |=
    OpBitXorAssign,     // ^=
    OpLeftShiftAssign,  // <<=
    OpRightShiftAssign, // >>=
};

OpCode toOpCode(Lexical::TokenType) noexcept;

} // namespace Semantic
} // namespace Compiler

#endif // OPCODE_HPP
