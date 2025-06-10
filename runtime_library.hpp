#pragma once

#include <string>

namespace lib {

class ArrayDouble;
class Runtime;

auto stddev(ArrayDouble* array) -> double;
auto mean(ArrayDouble* array) -> double;
auto count(ArrayDouble* array) -> double;
auto min(ArrayDouble* array) -> double;
auto max(ArrayDouble* array) -> double;
auto sin(double number) -> double;
auto cos(double number) -> double;
auto tan(double number) -> double;
auto pi() -> double;
auto atan(double number) -> double;
auto asin(double number) -> double;
auto acos(double number) -> double;
auto exp(double number) -> double;
auto ln(double number) -> double;
auto print(std::string& str) -> double;
auto random(double number) -> double;
auto len(std::string& str) -> double;
auto right(Runtime* runtime, std::string& str, double n) -> std::string*;
auto left(Runtime* runtime, std::string& str, double n) -> std::string*;

} // namespace lib
