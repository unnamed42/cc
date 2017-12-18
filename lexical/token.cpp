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

void Token::print(Logger &log) const { log << toString(m_type); }

Token* impl::makeToken(SourceLoc *source, TokenType type) {
    return new (pool) Token(source, type);
}

Token* impl::makeToken(SourceLoc *source, TokenType type, UString *str) {
    return new (pool) ContentToken(source, type, str);
}
