#include "driver.hpp"
#include "parser.hpp"
#include "scanner.hpp"

using namespace std;

auto Driver::parse(const std::string& file_name) -> int {
    ifstream input(file_name);

    if (!input.is_open()) {
        println("File not found!");
        return 1;
    }

    Lexer lexer;
    lexer.switch_streams(&input);

    output = ofstream("project02syn.txt");
    dgeval::Parser parser(*this, lexer);

    return parser();
}

void Driver::report() {
    println(output, "{}", assignment_count);
    println(output, "{}", evaluation_count);
    println(output, "{}", conditional_count);
    println(output, "{}", array_count);
    println(output, "{}", numeric_count);
    println(output, "{}", string_count);
    println(output, "{}", array_access_count);
    println(output, "{}", function_call_count);

    output.close();
}
