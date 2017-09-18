#include "mempool.hpp"
#include "lexical/token.hpp"

#include <new>
#include <cstring>

namespace impl = Compiler::Lexical;

using namespace Compiler;
using namespace Compiler::Lexical;
using namespace Compiler::Diagnostic;

Token::Token(SourceLoc *source, TokenType type) 
    : loc(source), type(type) {}

bool Token::is(TokenType type) const noexcept {
    return this->type == type;
}

Token* impl::makeToken(SourceLoc *source, TokenType type) {
    return new (pool.allocate(sizeof(Token))) Token(source, type);
}
