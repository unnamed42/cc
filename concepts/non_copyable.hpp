#ifndef NON_COPYABLE_HPP
#define NON_COPYABLE_HPP

namespace compiler {

struct non_copyable {
    non_copyable() = default;
    
    non_copyable(const non_copyable&) = delete;
    non_copyable& operator=(const non_copyable&) = delete;
};

} // namespace compiler

#endif // NON_COPYABLE_HPP
