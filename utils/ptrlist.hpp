#ifndef PTRLIST_HPP
#define PTRLIST_HPP

#include "utils/vector.hpp"

namespace Compiler {
namespace Utils {

extern template class Vector<void*>;

class PtrList : public Vector<void*> {
    private:
        using self = PtrList;
        using base = Vector<void*>;
    public:
        using base::base;
};

} // namespace Utils
} // namespace Compiler

#endif // PTRLIST_HPP
