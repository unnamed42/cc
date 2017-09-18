#include "mempool.hpp"
#include "sourceloc.hpp"

#include <cstring>

namespace impl = Compiler;

using namespace Compiler;

SourceLoc::SourceLoc(const char *path, FILE *file) 
    : path(path), file(file),
      lineBegin(0), line(1), column(1), length(0)  {}

SourceLoc::SourceLoc(const char *path, FILE *file,
                     long lineBegin, unsigned line, unsigned column, unsigned length) 
    : path(path), file(file), 
      lineBegin(lineBegin), line(line), column(column), length(length) {}

SourceLoc* impl::makeSourceLoc(const char *path,
                               FILE *file,
                               long lineBegin,
                               unsigned line,
                               unsigned column,
                               unsigned length) {
    auto *p = pool.allocate<SourceLoc>();
    return new (p) SourceLoc{path, file, lineBegin, line, column, length};
}

SourceLoc* impl::makeSourceLoc(const SourceLoc *source) {
    auto *p = pool.allocate<SourceLoc>();
    memcpy(p, source, sizeof(*p));
    return p;
}
