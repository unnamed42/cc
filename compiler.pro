TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CXXFLAGS = -std=c++11

SOURCES += main.cpp \
    error.cpp \
    token.cpp \
    cpp.cpp\
    evaluator.cpp \
    parser.cpp \
    type.cpp \
    scope.cpp \
    ast.cpp \
    lexer.cpp \
    codegen.cpp

HEADERS += \ 
    error.hpp \
    token.hpp \
    cpp.hpp\
    evaluator.hpp \
    parser.hpp \
    ast.hpp \
    type.hpp \
    scope.hpp \
    mempool.hpp \
    lexer.hpp \
    visitor.hpp \
    codegen.hpp \
    concepts/non_copyable.hpp

DISTFILES += \
    deprecated.txt
