#include <print>
#include "checker.hpp"
#include "dependency.hpp"
#include "driver.hpp"
#include "printer.hpp"

using namespace std;

auto main(int argc, char** argv) -> int {
    if (argc != 2) {
        println("Exactly one input file needs to be provided.");
        return 1;
    }

    Driver driver;

    int res = driver.parse(argv[1]);

    dgeval::ast::Dependency dependency;
    dgeval::ast::Checker checker;
    dgeval::ast::Printer printer(argv[1]);

    driver.program->accept(dependency);
    driver.program->accept(checker);
    driver.program->accept(printer);

    return res;
}
