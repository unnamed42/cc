#ifndef COMMON_HPP
#define COMMON_HPP

#include <cstdint>

#define NO_COPY(classname) \
    classname(const classname&) = delete; \
    classname& operator=(const classname&) = delete

#define NO_MOVE(classname) \
    classname(classname&&) = delete; \
    classname& operator=(classname&&) = delete

#define NO_COPY_MOVE(classname) \
    NO_COPY(classname); \
    NO_MOVE(classname)

#endif // COMMON_HPP
