#include "driver.hpp"
#include <print>
#include "parser.hpp"
#include "scanner.hpp"

using namespace std;

auto Driver::parse(const std::string& file_name) -> int {
    ifstream input(file_name + ".txt");

    if (!input.is_open()) {
        println("File not found!");
        return 1;
    }

    Lexer lexer;
    lexer.switch_streams(&input);

    dgeval::Parser parser(*this, lexer);

    return parser();
}
