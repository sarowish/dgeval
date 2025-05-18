#pragma once

#include <fstream>
#include <memory>
#include "ast.hpp"
#include "parser.hpp"

class Driver {
  public:
    auto parse(const std::string& file_name) -> int;
    void report();

    int assignment_count {};
    int evaluation_count {};
    int conditional_count {};
    int array_count {};
    int numeric_count {};
    int string_count {};
    int array_access_count {};
    int function_call_count {};

    std::unique_ptr<dgeval::ast::Program> program;
    std::ofstream output;
    std::string buffer;
    std::string raw_buffer;
    dgeval::location location;
};
