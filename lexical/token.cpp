#include "utils/mempool.hpp"
#include "text/ustring.hpp"
#include "lexical/token.hpp"
#include "lexical/tokentype.hpp"
#include "diagnostic/logger.hpp"

#include <cstring>

namespace impl = Compiler::Lexical;

using namespace Compiler::Text;
using namespace Compiler::Utils;
using namespace Compiler::Lexical;
using namespace Compiler::Diagnostic;

Token::Token(SourceLoc *source, TokenType type) noexcept : m_loc(source), m_type(type) {}
bool Token::is(TokenType type) const noexcept { return this->m_type == type; }
void Token::print(Logger &log) const { log << toString(m_type); }
const SourceLoc* Token::sourceLoc() const noexcept { return m_loc; }
TokenType Token::type() noexcept { return m_type; }
const UString* Token::content() const noexcept { return nullptr; }

ContentToken::ContentToken(SourceLoc *source, TokenType type, UString *str) : Token(source, type), content_(str) {}
void ContentToken::print(Logger &log) const { log << *content_; }
const UString* ContentToken::content() const noexcept { return content_; }

Token* impl::makeToken(SourceLoc *source, TokenType type) {
    return new (pool) Token(source, type);
}

Token* impl::makeToken(SourceLoc *source, TokenType type, UString *str) {
    return new (pool) ContentToken(source, type, str);
}
