cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")

project(cc)

include_directories(${${PROJECT_NAME}_SOURCE_DIR})

set(COMMON_FILES main.cpp common.hpp)

set(UTILS_FILES 
        utils/iterator.hpp
        utils/vector.hpp utils/vector.cpp
        utils/mempool.hpp utils/mempool.cpp)

set(TEXT_FILES 
        text/uchar.hpp text/uchar.cpp
        text/buffer.hpp text/buffer.cpp
        text/ustring.hpp text/ustring.cpp
        text/file.hpp text/file.cpp
        text/stream.hpp text/stream.cpp)

set(DIAGNOSTIC_FILES 
        diagnostic/logger.hpp diagnostic/logger.cpp
        diagnostic/sourceloc.hpp diagnostic/sourceloc.cpp)

set(LEXICAL_FILES 
        lexical/tokentype.hpp lexical/tokentype.cpp
        lexical/token.hpp lexical/token.cpp
        lexical/lexer.hpp lexical/lexer.cpp
        lexical/pp.hpp lexical/pp.cpp)

set(SEMANTIC_FILES
        semantic/opcode.hpp semantic/opcode.cpp
        semantic/decl.hpp semantic/decl.cpp
        semantic/expr.hpp semantic/expr.cpp
        semantic/stmt.hpp semantic/stmt.cpp
        semantic/typeenum.hpp semantic/typeenum.cpp
        semantic/type.hpp semantic/type.cpp
        semantic/qualtype.hpp semantic/qualtype.cpp
        semantic/scope.hpp semantic/scope.cpp
        semantic/parser.hpp semantic/parser.cpp)

set(CONSTEXPR_FILES 
        constexpr/value.hpp constexpr/value.cpp
        constexpr/evaluator.hpp constexpr/evaluator.cpp)

set(SOURCE_FILES 
        ${COMMON_FILES} ${UTILS_FILES} ${TEXT_FILES} ${DIAGNOSTIC_FILES}
        ${LEXICAL_FILES} ${SEMANTIC_FILES} ${CONSTEXPR_FILES})

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
