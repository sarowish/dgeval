#include "driver.hpp"
#include "parser.hpp"
#include "scanner.hpp"

auto Driver::parse(std::ifstream& input) -> int {
    Lexer lexer;
    lexer.switch_streams(&input);

    dgeval::Parser parser(*this, lexer);

    return parser();
}
