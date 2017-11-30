#include "semantic/parser.hpp"

#include <cassert>

using namespace Compiler::Semantic;

int main(int argc, char* argv[]) {
    assert(argc == 2);
    
    Parser parser{argv[1]};
    parser.parse();
}
