#pragma once

#include <fstream>
#include "parser.hpp"

class Driver {
  public:
    auto parse(std::ifstream& input) -> int;

    std::unique_ptr<dgeval::ast::Program> program;
    std::string buffer;
    dgeval::location location;
};
