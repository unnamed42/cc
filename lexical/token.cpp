#include "token.hpp"
#include "mempool.hpp"

#include <cstring>

using namespace Compiler;

bool Token::is(TokenType type) const noexcept {
    return this->type == type;
}

Token* makeToken(const SourceLoc *source, TokenType type) {
    auto p = pool.allocate<Token>();
    memcpy(&p->loc, source, sizeof(*source));
    p->type = type;
    return p;
}
