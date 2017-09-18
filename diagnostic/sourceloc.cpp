#include "mempool.hpp"
#include "diagnostic/sourceloc.hpp"

#include <cstring>

namespace impl = Compiler::Diagnostic;

using namespace Compiler::Diagnostic;

SourceLoc::SourceLoc(const char *path, FILE *file) 
    : path(path), file(file),
      lineBegin(0), line(1), column(1), length(0)  {}

SourceLoc* impl::makeSourceLoc(const char *path,
                               FILE *file,
                               SourceLoc::PosType lineBegin,
                               unsigned line,
                               unsigned column,
                               unsigned length) {
    auto *p = pool.allocate<SourceLoc>();
    p->path = path;
    p->file = file;
    p->lineBegin = lineBegin;
    p->line = line;
    p->column = column;
    p->length = length;
    return p;
}

SourceLoc* impl::makeSourceLoc(const SourceLoc *source) {
    auto *p = pool.allocate<SourceLoc>();
    memcpy(p, source, sizeof(*p));
    return p;
}
