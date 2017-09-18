#ifndef CC_DEBUG
#define CC_DEBUG
#endif

#include "parser.hpp"

#include <iostream>

int main() try {
    compiler::parser parser("/home/h/2.c");
    parser.process();
    parser.print();
    return EXIT_SUCCESS;
} catch (int) {
    return EXIT_FAILURE;
}
