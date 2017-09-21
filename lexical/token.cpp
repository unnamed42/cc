#include "utils/mempool.hpp"
#include "text/ustring.hpp"
#include "lexical/token.hpp"
#include "lexical/tokentype.hpp"

#include <cstring>

namespace impl = Compiler::Lexical;

using namespace Compiler::Text;
using namespace Compiler::Utils;
using namespace Compiler::Lexical;
using namespace Compiler::Diagnostic;

Token::Token(SourceLoc *source, TokenType type) noexcept : m_loc(source), m_type(type) {}
bool Token::is(TokenType type) const noexcept { return this->m_type == type; }
UString Token::toString() const { return {::toString(m_type)}; }
SourceLoc* Token::sourceLoc() noexcept { return m_loc; }
TokenType Token::type() noexcept { return m_type; }

ContentToken::ContentToken(SourceLoc *source, TokenType type, UString *str) : Token(source, type), content_(str) {}
UString ContentToken::toString() const { return *content_; }
const UString* ContentToken::content() const noexcept { return content_; }

Token* impl::makeToken(SourceLoc *source, TokenType type) {
    return new (pool.allocate(sizeof(Token))) Token(source, type);
}

Token* impl::makeToken(SourceLoc *source, TokenType type, UString *str) {
    return new (pool.allocate(sizeof(ContentToken))) ContentToken(source, type, str);
}
