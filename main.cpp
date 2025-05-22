#include <print>
#include "checker.hpp"
#include "dependency.hpp"
#include "driver.hpp"
#include "printer.hpp"

auto main(int argc, char** argv) -> int {
    if (argc != 2) {
        std::println("Exactly one input file needs to be provided.");
        return 1;
    }

    std::string file_name = std::string(argv[1]);
    std::ifstream input(file_name + ".txt");

    if (!input.is_open()) {
        std::println("File not found!");
        return 1;
    }

    Driver driver;
    int res = driver.parse(input);

    if (res == 0) {
        dgeval::ast::Dependency dependency;
        driver.program->accept(dependency);
        dgeval::ast::Checker checker;
        driver.program->accept(checker);
    }

    dgeval::ast::Printer printer(file_name);
    driver.program->accept(printer);

    return res;
}
