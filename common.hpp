#ifndef COMMON_HPP
#define COMMON_HPP

#include <cstdint>

#define NO_COPY_MOVE(classname) \
    classname(const classname&) = delete; \
    classname(classname&&) = delete; \
    classname& operator=(const classname&) = delete; \
    classname& operator=(classname&&) = delete

#endif // COMMON_HPP
