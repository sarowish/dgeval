#include "runtime_library.hpp"
#include <cmath>
#include <iostream>
#include <random>
#include "lang_runtime.hpp"

namespace lib {

auto stddev(ArrayDouble* array) -> double {
    return array->stddev();
}

auto mean(ArrayDouble* array) -> double {
    return array->mean();
}

auto count(ArrayDouble* array) -> double {
    return array->count();
}

auto min(ArrayDouble* array) -> double {
    return array->min();
}

auto max(ArrayDouble* array) -> double {
    return array->max();
}

auto print(std::string& str) -> double {
    std::cout << str;
    return str.length();
}

auto sin(double number) -> double {
    return std::sin(number);
}

auto cos(double number) -> double {
    return std::cos(number);
}

auto tan(double number) -> double {
    return std::tan(number);
}

auto pi() -> double {
    return M_PI;
}

auto atan(double number) -> double {
    return std::atan(number);
}

auto asin(double number) -> double {
    return std::asin(number);
}

auto acos(double number) -> double {
    return std::acos(number);
}

auto exp(double number) -> double {
    return std::exp(number);
}

auto ln(double number) -> double {
    return std::log(number);
}

auto random(double number) -> double {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> distr(0, number);
    return distr(gen);
}

auto len(std::string& str) -> double {
    return str.size();
}

auto right(Runtime* runtime, std::string& str, double n) -> std::string* {
    auto* new_str = new std::string(str.substr(str.length() - n));

    runtime->register_string_object(new_str);

    return new_str;
}

auto left(Runtime* runtime, std::string& str, double n) -> std::string* {
    auto* new_str = new std::string(str.substr(0, n));

    runtime->register_string_object(new_str);

    return new_str;
}

} // namespace lib
