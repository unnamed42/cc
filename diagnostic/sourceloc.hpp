#ifndef SOURCELOC_HPP
#define SOURCELOC_HPP

#include <cstdio>

namespace Compiler {
namespace Diagnostic {

struct SourceLoc {
    /**< return type of ftell() */
    using PosType = long;
    
    /**< path to this source content, if nullptr means this is generated from macro */
    const char *path;
    /**< file device */
    FILE *file;
    /**< line beginning position of this source content */
    PosType lineBegin;
    /**< line number, starting from 1, for diagnostic only */
    unsigned line;
    /**< column, starting from 1 */
    unsigned column;
    /**< length of this source content, used only in diagnostics */
    unsigned length;
    
    SourceLoc() = default;
    SourceLoc(const char *path, FILE *file);
};

SourceLoc* makeSourceLoc(const SourceLoc *source);

} // namespace Diagnostic
} // namespace Compiler

#endif // SOURCELOC_HPP
