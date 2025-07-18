#include "lang_runtime.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace lib {

ArrayString::ArrayString(std::string** base, int count) : ArrayString() {
    for (int idx = count - 1; idx >= 0; --idx) {
        inner->push_back(base[idx]);
    }
}

auto ArrayString::elements_equal(std::string* p1, std::string* p2) const
    -> bool {
    return *p1 == *p2;
}

ArrayDouble::ArrayDouble(double* base, int count) : ArrayDouble() {
    for (int idx = count - 1; idx >= 0; --idx) {
        inner->push_back(base[idx]);
    }
}

auto ArrayDouble::elements_equal(double p1, double p2) const -> bool {
    return p1 == p2;
}

ArrayBool::ArrayBool(int64_t* base, int count) : ArrayBool() {
    for (int idx = count - 1; idx >= 0; --idx) {
        inner->push_back(static_cast<bool>(base[idx]));
    }
}

auto ArrayBool::elements_equal(bool p1, bool p2) const -> bool {
    return p1 == p2;
}

ArrayArray::ArrayArray(TypeDescriptor type, ArrayArray** base, int count) :
    ArrayArray(type) {
    for (int idx = count - 1; idx >= 0; --idx) {
        inner->push_back(base[idx]);
    }
}

auto ArrayArray::elements_equal(ArrayArray* p1, ArrayArray* p2) const -> bool {
    return p1->equals_to(p2);
}

void Runtime::register_array_object(Array* array) {
    arrays.push_back(array);
}

void Runtime::register_string_object(std::string* str) {
    strings.push_back(str);
}

auto Runtime::allocate_array(
    Runtime* runtime,
    TypeDescriptor type,
    int len,
    uint64_t* base
) -> Array* {
    Array* array {};

    if (type.dimension == 1) {
        switch (type.type) {
            case Type::Boolean:
                array = new ArrayBool(std::bit_cast<int64_t*>(base), len);
                break;
            case Type::Number:
                array = new ArrayDouble(std::bit_cast<double*>(base), len);
                break;
            case Type::String:
                array =
                    new ArrayString(std::bit_cast<std::string**>(base), len);
                break;
            default:
                break;
        }
    } else {
        array = new ArrayArray(type, std::bit_cast<ArrayArray**>(base), len);
    }

    runtime->register_array_object(array);

    return array;
}

auto Runtime::array_element(Runtime* runtime, Array* array, int64_t index)
    -> uint64_t {
    if (array->type.is_array()) {
        auto* array_array = dynamic_cast<ArrayArray*>(array);

        runtime->exception = index < 0
            || index >= static_cast<int64_t>(array_array->inner->size());

        if (!runtime->exception) {
            return std::bit_cast<uint64_t>((*array_array->inner)[index]);
        }
    } else {
        switch (array->type.type) {
            case Type::Boolean: {
                auto* bool_array = dynamic_cast<ArrayBool*>(array);

                runtime->exception = index < 0
                    || index >= static_cast<int64_t>(bool_array->inner->size());

                if (!runtime->exception) {
                    return static_cast<uint64_t>((*bool_array->inner)[index]);
                }
            } break;
            case Type::String: {
                auto* string_array = dynamic_cast<ArrayString*>(array);

                runtime->exception = index < 0
                    || index
                        >= static_cast<int64_t>(string_array->inner->size());

                if (!runtime->exception) {
                    return std::bit_cast<uint64_t>(
                        (*string_array->inner)[index]
                    );
                }
            } break;
            case Type::Number: {
                auto* double_array = dynamic_cast<ArrayDouble*>(array);

                runtime->exception = index < 0
                    || index
                        >= static_cast<int64_t>(double_array->inner->size());

                if (!runtime->exception) {
                    return std::bit_cast<uint64_t>(
                        (*double_array->inner)[index]
                    );
                }
            } break;
            default:
                break;
        }
    }

    return 0;
}

auto Runtime::append_element(Array* array, uint64_t value) -> Array* {
    if (array->type.is_array()) {
        (dynamic_cast<ArrayArray*>(array))
            ->inner->push_back(std::bit_cast<ArrayArray*>(value));
    } else {
        switch (array->type.type) {
            case Type::Boolean:
                (dynamic_cast<ArrayBool*>(array))
                    ->inner->push_back(static_cast<bool>(value));
                break;
            case Type::String:
                (dynamic_cast<ArrayString*>(array))
                    ->inner->push_back(std::bit_cast<std::string*>(value));
                break;
            case Type::Number: {
                double number = *std::bit_cast<double*>(&value);
                (dynamic_cast<ArrayDouble*>(array))->inner->push_back(number);
            } break;
            default:
                break;
        }
    }

    return array;
}

auto Runtime::allocate_string(Runtime* runtime, std::string* str)
    -> std::string* {
    auto* p = new std::string(*str);

    runtime->register_string_object(p);

    return p;
}

auto Runtime::cat_string(Runtime* runtime, std::string* s1, std::string* s2)
    -> std::string* {
    auto* p = new std::string(*s1 + *s2);

    runtime->register_string_object(p);

    return p;
}

auto Runtime::number_to_string(Runtime* runtime, double number)
    -> std::string* {
    auto* p = new std::string(std::to_string(number));

    runtime->register_string_object(p);

    return p;
}

auto Runtime::strcmp(std::string* s1, std::string* s2, int64_t comparison)
    -> int64_t {
    int ordering = s1->compare(*s2);

    switch (comparison) {
        case 0:
            return ordering == 0;
        case 1:
            return ordering != 0;
        case 2:
            return ordering > 0;
        case 3:
            return ordering < 0;
        case 4:
            return ordering >= 0;
        case 5:
            return ordering <= 0;
        default:
            return 0;
    }
}

auto Runtime::arrcmp(Array* arr1, Array* arr2) -> int64_t {
    return arr1->_equals_to(arr2);
}

auto Runtime::post_exec_cleanup(Runtime* runtime) -> int64_t {
    for (auto* str : runtime->strings) {
        delete (str);
    }

    for (auto* arr : runtime->arrays) {
        delete (arr);
    }

    return true;
}

auto Runtime::check_exception(Runtime* runtime) -> int64_t {
    return runtime->exception;
}

auto ArrayDouble::stddev() -> double {
    size_t len = inner->size();

    if (len == 0) {
        return 0;
    }

    double sumx = 0;
    double sumx2 = 0;

    for (auto number : *inner) {
        sumx += number;
        sumx2 += number * number;
    }

    double mean = sumx / len;
    double variance = sumx2 / len - mean * mean;

    return std::sqrt(variance);
}

auto ArrayDouble::mean() -> double {
    size_t len = inner->size();

    if (len == 0) {
        return 0;
    }

    double sum = std::reduce(inner->begin(), inner->end());
    return sum / len;
}

auto ArrayDouble::count() -> double {
    return inner->size();
}

auto ArrayDouble::min() -> double {
    return *std::min_element(inner->begin(), inner->end());
}

auto ArrayDouble::max() -> double {
    return *std::max_element(inner->begin(), inner->end());
}

} // namespace lib
