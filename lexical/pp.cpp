#include "utils/mempool.hpp"
#include "text/ustring.hpp"
#include "lexical/pp.hpp"
#include "lexical/token.hpp"
#include "lexical/tokentype.hpp"
#include "diagnostic/logger.hpp"

using namespace Compiler::Utils;
using namespace Compiler::Lexical;
using namespace Compiler::Diagnostic;

PP::PP(const char *path) : m_unget(), m_src(path) {}

Token* PP::get() {
    if(!m_unget.empty())
        return static_cast<Token*>(m_unget.popBack());
    return m_src.get();
}

Token* PP::peek() {
    auto ret = get();
    unget(ret);
    return ret;
}

void PP::unget(Token *tok) {
    m_unget.pushBack(tok);
}

Token* PP::want(TokenType type) {
    auto ret = get();
    if(ret && !ret->is(type)) {
        unget(ret);
        return nullptr;
    }
    return ret;
}

Token* PP::want(bool (*checker)(TokenType)) {
    auto ret = get();
    if(ret && !checker(ret->type())) {
        unget(ret);
        return nullptr;
    }
    return ret;
}

bool PP::test(TokenType type) {
    auto tok = want(type);
    pool.deallocate(tok);
    return tok != nullptr;
}

void PP::expect(TokenType type) {
    auto ret = get();
    if(!ret) 
        derr << "unexpected end of file";
    if(!ret->is(type)) 
        derr << ret->sourceLoc()
            << "expecting " << type << ", but get " << ret;
    pool.deallocate(ret);
}
