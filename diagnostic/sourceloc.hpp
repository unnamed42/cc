#ifndef SOURCELOC_HPP
#define SOURCELOC_HPP

#include <cstdio>

namespace Compiler {
namespace Diagnostic {

struct SourceLoc {
    /**< return type of ftell() */
    using PosType = decltype(ftell(0));
    
    /**< path to this source content, if nullptr means this is generated from macro */
    const char *path;
    /**< file device */
    FILE *file;
    /**< line beginning position of this source content */
    PosType lineBegin = 0;
    /**< line number, starting from 1, for diagnostic only */
    unsigned line = 1;
    /**< column, starting from 1 */
    unsigned column = 1;
    /**< length of this source content, used only in diagnostics */
    unsigned length = 0;
    
    SourceLoc() noexcept = default;
    
    SourceLoc(const char *path, FILE *file) noexcept
        : path(path), file(file) {}
    
    /**
     * Make a clone of this object
     */
    SourceLoc* clone() const;
};

} // namespace Diagnostic
} // namespace Compiler

#endif // SOURCELOC_HPP
