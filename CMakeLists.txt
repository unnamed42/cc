cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")

project(compiler_)

include_directories(${${PROJECT_NAME}_SOURCE_DIR})

set(SOURCE_FILES 
        main.cpp
        common.hpp
        text/uchar.hpp text/uchar.cpp
        text/ustring.hpp text/ustring.cpp
        text/file.hpp text/file.cpp
        text/stream.hpp text/stream.cpp
        mempool.hpp mempool.cpp
        diagnostic.hpp diagnostic.cpp
        sourceloc.hpp sourceloc.cpp
        lexical/tokentype.hpp lexical/tokentype.cpp
        lexical/token.hpp lexical/token.cpp
        lexical/lexer.hpp lexical/lexer.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
