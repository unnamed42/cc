cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")

project(cc)

include_directories(${${PROJECT_NAME}_SOURCE_DIR})

set(SOURCE_FILES 
        main.cpp
        common.hpp
        utils/mempool.hpp utils/mempool.cpp
        utils/iterator.hpp
        utils/list.hpp utils/list.cpp
        text/uchar.hpp text/uchar.cpp
        text/ustring.hpp text/ustring.cpp
        text/file.hpp text/file.cpp
        text/stream.hpp text/stream.cpp
        diagnostic/printer.hpp diagnostic/printer.cpp
        diagnostic/sourceloc.hpp diagnostic/sourceloc.cpp
        lexical/tokentype.hpp lexical/tokentype.cpp
        lexical/token.hpp lexical/token.cpp
        lexical/lexer.hpp lexical/lexer.cpp
        semantic/ast.hpp semantic/ast.cpp
        semantic/type.hpp semantic/type.cpp
        semantic/typeenum.hpp semantic/typeenum.cpp
        semantic/qualtype.hpp semantic/qualtype.cpp
        semantic/scope.hpp semantic/scope.cpp
        semantic/parser.hpp semantic/parser.cpp
        constexpr/value.hpp constexpr/value.cpp
        constexpr/evaluator.hpp constexpr/evaluator.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
