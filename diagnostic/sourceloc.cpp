#include "utils/mempool.hpp"
#include "diagnostic/sourceloc.hpp"

#include <cstring>

using namespace Compiler::Utils;
using namespace Compiler::Diagnostic;

SourceLoc* SourceLoc::clone() const {
    return static_cast<SourceLoc*>(
        memcpy(pool.allocate(sizeof(SourceLoc)), this, sizeof(*this))
    );
}
