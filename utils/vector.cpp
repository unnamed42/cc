#include "utils/vector.hpp"

using namespace Compiler::Utils;
using namespace Compiler::Lexical;
using namespace Compiler::Semantic;

template class Compiler::Utils::Vector<Token*>;
template class Compiler::Utils::Vector<Decl*>;
template class Compiler::Utils::Vector<Expr*>;
