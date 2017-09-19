#include "semantic/scope.hpp"
#include "semantic/parser.hpp"

namespace impl = Compiler::Semantic;

using namespace Compiler::Text;
using namespace Compiler::Semantic;
using namespace Compiler::Diagnostic;

SourceLoc *impl::epos = nullptr;
