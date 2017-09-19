#include "mempool.hpp"
#include "text/ustring.hpp"
#include "lexical/token.hpp"

#include <new>
#include <cstring>

namespace impl = Compiler::Lexical;

using namespace Compiler;
using namespace Compiler::Text;
using namespace Compiler::Lexical;
using namespace Compiler::Diagnostic;

Token::Token(SourceLoc *source, TokenType type) 
    : loc(source), type(type) {}

bool Token::is(TokenType type) const noexcept {
    return this->type == type;
}

ContentToken::ContentToken(SourceLoc *source, TokenType type, UString &&str) 
    : Token(source, type), content_(clone(std::move(str))) {}

UString* ContentToken::content() {
    return content_;
}


Token* impl::makeToken(SourceLoc *source, TokenType type) {
    return new (pool.allocate(sizeof(Token))) Token(source, type);
}

Token* impl::makeToken(SourceLoc *source, TokenType type, UString &&str) {
    return new (pool.allocate(sizeof(ContentToken))) ContentToken(source, type, std::move(str));
}
