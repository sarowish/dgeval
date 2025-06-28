#include <charconv>
#include <print>
#include "checker.hpp"
#include "codegen.hpp"
#include "dependency.hpp"
#include "driver.hpp"
#include "fold.hpp"
#include "optimize.hpp"
#include "printer.hpp"

auto main(int argc, char** argv) -> int {
    if (argc != 2 && argc != 3) {
        std::println(
            "Usage is {} <optional optimization parameter> <dgeval module file name",
            argv[0]
        );
        return 1;
    }

    dgeval::ast::OptimizationFlags optimization;

    if (argc == 3) {
        std::string flag = argv[1];

        if (flag.length() <= 2 || !flag.starts_with("-p")) {
            std::println("Invalid optimization flag.");
            return 1;
        }

        int parameter {};
        auto result = std::from_chars(
            flag.data() + 2,
            flag.data() + flag.size(),
            parameter
        );

        if (result.ec != std::errc()) {
            std::println("-p flag must be followed by a valid integer.");
            return 1;
        }

        if (parameter < 0 || parameter > 0b1111) {
            std::println(
                "Invalid optimization value after -p. It must be between 0 and 15."
            );
            return 1;
        }

        optimization = dgeval::ast::OptimizationFlags(parameter);
    }

    std::string file_name = std::string(argv[argc - 1]);
    std::ifstream input(file_name + ".txt");

    if (!input.is_open()) {
        std::println("File not found!");
        return 1;
    }

    Driver driver;
    dgeval::ast::Printer printer(file_name);
    int res = driver.parse(input);

    if (res == 0) {
        dgeval::ast::Dependency dependency;
        driver.program->accept(dependency);
        dgeval::ast::Checker checker;
        driver.program->accept(checker);
    }

    if (!driver.program->any_errors()) {
        dgeval::ast::Fold folder;
        driver.program->accept(folder);
        if (!driver.program->any_errors()) {
            dgeval::ast::LinearIR ic(optimization);
            driver.program->accept(ic);
            dgeval::ast::Peephole peephole(
                driver.program->instructions,
                optimization
            );
            peephole.run();
            print_ic(file_name + "-IC.txt", driver.program->instructions);
        }
    }

    driver.program->messages.emplace_back("Completed compilation");
    driver.program->accept(printer);

    if (!driver.program->any_errors()) {
        Codegen codegen;
        DynamicFunction* func = codegen.generate(*driver.program);

        if (func) {
            func();
        }
    }

    return res;
}
