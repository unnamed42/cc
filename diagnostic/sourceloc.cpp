#include "utils/mempool.hpp"
#include "diagnostic/sourceloc.hpp"

#include <cstring>

namespace impl = Compiler::Diagnostic;

using namespace Compiler::Utils;
using namespace Compiler::Diagnostic;

SourceLoc::SourceLoc(const char *path, FILE *file) noexcept
    : path(path), file(file) {}

SourceLoc* SourceLoc::clone() const {
    auto p = static_cast<SourceLoc*>(pool.allocate(sizeof(SourceLoc)));
    memcpy(p, this, sizeof(*this));
    return p;
}
