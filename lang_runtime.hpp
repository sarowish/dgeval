#pragma once

#include <string>
#include <vector>
#include "ast.hpp"

using dgeval::ast::BOOLEAN;
using dgeval::ast::NUMBER;
using dgeval::ast::STRING;
using dgeval::ast::Type;
using dgeval::ast::TypeDescriptor;

namespace lib {

class Array {
  public:
    Array(TypeDescriptor type) : type(type) {}

    virtual ~Array() = default;

    virtual auto _equals_to(Array* other) -> bool = 0;

    TypeDescriptor type;
};

template<typename T>
class ArrayType: public Array {
  public:
    ArrayType(TypeDescriptor type) : Array(type), inner(new std::vector<T>) {}

    ~ArrayType() {
        delete inner;
    }

    [[nodiscard]] virtual auto elements_equal(T p1, T p2) const -> bool = 0;

    virtual auto _equals_to(Array* other) -> bool {
        return equals_to((ArrayType<T>*)other);
    }

    [[nodiscard]] virtual auto equals_to(ArrayType<T>* other) const -> bool {
        int len = inner->size();
        std::vector<T>* other_array = other->inner;

        if (len != other_array->size()) {
            return false;
        }

        for (size_t idx = 0; idx < len; ++idx) {
            if (!elements_equal((*inner)[idx], (*other_array)[idx])) {
                return false;
            }
        }

        return true;
    }

    std::vector<T>* inner;
};

class ArrayString: public ArrayType<std::string*> {
  public:
    ArrayString() : ArrayType<std::string*>(STRING) {}

    ArrayString(std::string** base, int count);

    [[nodiscard]] virtual auto
    elements_equal(std::string* p1, std::string* p2) const -> bool override;
};

class ArrayDouble: public ArrayType<double> {
  public:
    ArrayDouble() : ArrayType<double>(NUMBER) {}

    ArrayDouble(double* base, int count);

    [[nodiscard]] virtual auto elements_equal(double p1, double p2) const
        -> bool override;

    auto stddev() -> double;
    auto mean() -> double;
    auto count() -> double;
    auto min() -> double;
    auto max() -> double;
};

class ArrayBool: public ArrayType<bool> {
  public:
    ArrayBool() : ArrayType<bool>(BOOLEAN) {}

    ArrayBool(int64_t* base, int count);

    [[nodiscard]] virtual auto elements_equal(bool p1, bool p2) const
        -> bool override;
};

class ArrayArray: public ArrayType<ArrayArray*> {
  public:
    ArrayArray(TypeDescriptor type) : ArrayType<ArrayArray*>(type) {}

    ArrayArray(TypeDescriptor type, ArrayArray** base, int count);

    [[nodiscard]] virtual auto
    elements_equal(ArrayArray* p1, ArrayArray* p2) const -> bool override;
};

class Runtime {
  public:
    void register_array_object(Array* array);
    void register_string_object(std::string* str);
    static auto allocate_array(
        Runtime* runtime,
        TypeDescriptor type,
        int len,
        uint64_t* base
    ) -> Array*;
    static auto array_element(Runtime* runtime, Array* array, int64_t index)
        -> uint64_t;
    static auto append_element(Array* array, uint64_t value) -> Array*;
    static auto allocate_string(Runtime* runtime, std::string* str)
        -> std::string*;
    static auto cat_string(Runtime* runtime, std::string* s1, std::string* s2)
        -> std::string*;
    static auto number_to_string(Runtime* runtime, double number)
        -> std::string*;
    static auto strcmp(std::string* s1, std::string* s2, int64_t comparision)
        -> int64_t;
    static auto arrcmp(Array* arr1, Array* arr2) -> int64_t;
    static auto post_exec_cleanup(Runtime* runtime) -> int64_t;
    static auto check_exception(Runtime* runtime) -> int64_t;

    std::vector<Array*> arrays;
    std::vector<std::string*> strings;
    bool exception {false};
};

} // namespace lib
