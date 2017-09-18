#ifndef SOURCELOC_HPP
#define SOURCELOC_HPP

#include <cstdio>

namespace Compiler {

struct SourceLoc {
    /**< path to this source content, if nullptr means this is generated from macro */
    const char *path;
    /**< file device */
    FILE *file;
    /**< line beginning position of this source content */
    long lineBegin;
    /**< line number, starting from 1, for diagnostic only */
    unsigned line;
    /**< column, starting from 1 */
    unsigned column;
    /**< length of this source content, used only in diagnostics */
    unsigned length;
    
    SourceLoc(const char *path, FILE *file);
    SourceLoc(const char *path,
              FILE *file,
              long lineBegin,
              unsigned line,
              unsigned column,
              unsigned length = 0);
};

SourceLoc* makeSourceLoc(const char *path,
                         FILE *file,
                         long lineBegin,
                         unsigned line,
                         unsigned column,
                         unsigned length = 0);

SourceLoc* makeSourceLoc(const SourceLoc *source);

} // namespace Compiler

#endif // SOURCELOC_HPP
