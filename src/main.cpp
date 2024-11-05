#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include <iostream>
#include <fstream>
#include <sstream>

namespace {
    std::string readFile(const std::string& path) {
        std::ifstream file(path);
        if (!file) {
            throw std::runtime_error("Could not open file: " + path);
        }
        return std::string(
            std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>()
        );
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <source_file>\n";
        return 1;
    }

    try {
        const auto source = readFile(argv[1]);
        
        Lexer lexer(source);
        auto tokens = lexer.scanTokens();
        
        Parser parser(tokens);
        auto ast = parser.parseCook();
        
        CodeGen codegen;
        codegen.generateCode(ast.get());
        codegen.executeCode();
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}