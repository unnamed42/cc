#ifndef COMMON_HPP
#define COMMON_HPP

#include <cstdint>

#define NO_COPY(classname) \
    public: \
        classname(const classname&) = delete; \
        classname& operator=(const classname&) = delete

#define NO_MOVE(classname) \
    public: \
        classname(classname&&) = delete; \
        classname& operator=(classname&&) = delete

#define NO_COPY_MOVE(classname) \
    public: \
        NO_COPY(classname); \
        NO_MOVE(classname)

namespace Compiler {

template <class T> struct removeRef { using type = T; };
template <class T> struct removeRef<T&> { using type = T; };
template <class T> struct removeRef<T&&> { using type = T; };

template <class T>
using RValRef = typename removeRef<T>::type &&;

template <class T>
inline RValRef<T> move(T &&value) noexcept { return static_cast<RValRef<T>>(value); }

} // namespace Compiler

#endif // COMMON_HPP
