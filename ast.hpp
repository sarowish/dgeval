#pragma once

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>
#include "location.hpp"
#include "visitor.hpp"

namespace dgeval::ast {

enum class Opcode : std::uint8_t {
    None = 0,
    Comma = 1,
    Assign = 2,
    Conditional = 3,
    Alt = 4,
    And = 5,
    Or = 6,
    Equal = 7,
    NotEqual = 8,
    Less = 9,
    LessEqual = 10,
    Greater = 11,
    GreaterEqual = 12,
    Add = 13,
    Subtract = 14,
    Multiply = 15,
    Divide = 16,
    Minus = 17,
    Not = 18,
    ArrayAccess = 19,
    Call = 20,
    Identifier = 23,
    Literal = 24,
};

const std::array<std::string, 25> MNEMONICS = {
    "nop", "comma", "assign", "cond", "alt", "band", "bor",  "eq",  "neq",
    "lt",  "lte",   "gt",     "gte",  "add", "sub",  "mul",  "div", "minus",
    "not", "aa",    "call",   "jmp",  "jf",  "id",   "const"
};

const std::array<std::string, 21> OPERATOR_SYMBOLS = {
    "",  ",",  "=", "?", ":", "&&", "||", "==", "!=", "<", "<=",
    ">", ">=", "+", "-", "*", "/",  "-",  "!",  "[]", "()"
};

enum class Type : std::uint8_t { None, Number, String, Boolean, Array };

const std::array<std::string, 5> TYPE_STR =
    {"none", "number", "string", "boolean", "array"};

class TypeDescriptor {
  public:
    auto operator==(TypeDescriptor other) const -> bool {
        return type == other.type && dimension == other.dimension;
    }

    auto operator!=(TypeDescriptor other) const -> bool {
        return type != other.type || dimension != other.dimension;
    }

    auto is_array() const -> bool {
        return dimension != 0;
    }

    auto is_empty_array() const -> bool {
        return type == Type::None && dimension == 1;
    }

    auto item_type() const -> TypeDescriptor {
        TypeDescriptor type = *this;
        --type.dimension;

        return type;
    }

    auto to_string() const -> std::string {
        auto s = TYPE_STR[std::to_underlying(type)];
        if (dimension != 0) {
            return std::format("[{}]", s);
        }
        return s;
    }

    Type type {};
    int dimension {};
};

inline constexpr TypeDescriptor NONE;
inline constexpr TypeDescriptor NUMBER(Type::Number);
inline constexpr TypeDescriptor STRING(Type::String);
inline constexpr TypeDescriptor BOOLEAN(Type::Boolean);

class FunctionSignature {
  public:
    TypeDescriptor return_type;
    int parameter_count;
    std::vector<TypeDescriptor> parameters;
};

const std::unordered_map<std::string, FunctionSignature> RUNTIME_LIBRARY = {
    {"stddev", {NUMBER, 1, {{Type::Number, 1}}}},
    {"mean", {NUMBER, 1, {{Type::Number, 1}}}},
    {"count", {NUMBER, 1, {{Type::Number, 1}}}},
    {"min", {NUMBER, 1, {{Type::Number, 1}}}},
    {"max", {NUMBER, 1, {{Type::Number, 1}}}},
    {"sin", {NUMBER, 1, {NUMBER}}},
    {"cos", {NUMBER, 1, {NUMBER}}},
    {"tan", {NUMBER, 1, {NUMBER}}},
    {"pi", {NUMBER, 0}},
    {"atan", {NUMBER, 1, {NUMBER}}},
    {"asin", {NUMBER, 1, {NUMBER}}},
    {"acos", {NUMBER, 1, {NUMBER}}},
    {"exp", {NUMBER, 1, {NUMBER}}},
    {"ln", {NUMBER, 1, {NUMBER}}},
    {"print", {NUMBER, 1, {STRING}}},
    {"random", {NUMBER, 1, {NUMBER}}},
    {"len", {NUMBER, 1, {STRING}}},
    {"right", {NUMBER, 2, {STRING, NUMBER}}},
    {"left", {NUMBER, 2, {STRING, NUMBER}}},
};

class Expression {
  public:
    Expression() = default;
    virtual ~Expression() = default;

    Expression(location& loc, Type type) :
        loc(loc),
        opcode(Opcode::Literal),
        type_desc(TypeDescriptor(type)) {}

    Expression(location& loc, Opcode opcode) : loc(loc), opcode(opcode) {}

    virtual void accept(Visitor& visitor) = 0;

    location loc;
    Opcode opcode {};
    TypeDescriptor type_desc;
};

class NumberLiteral: public Expression {
  public:
    NumberLiteral(location& loc, double value) :
        Expression(loc, Type::Number),
        value(value) {}

    void accept(Visitor& visitor) override {
        visitor.visit_number(*this);
    }

    double value;
};

class StringLiteral: public Expression {
  public:
    StringLiteral(location& loc, std::string value, std::string raw_value) :
        Expression(loc, Type::String),
        value(std::move(value)),
        raw_value(std::move(raw_value)) {}

    void accept(Visitor& visitor) override {
        visitor.visit_string(*this);
    }

    std::string value;
    std::string raw_value;
};

class BooleanLiteral: public Expression {
  public:
    BooleanLiteral(location& loc, bool value) :
        Expression(loc, Type::Boolean),
        value(value) {}

    void accept(Visitor& visitor) override {
        visitor.visit_boolean(*this);
    }

    bool value;
};

class ArrayLiteral: public Expression {
  public:
    ArrayLiteral(location& loc, std::unique_ptr<Expression> items) :
        Expression(loc, Type::Array),
        items(std::move(items)) {}

    void accept(Visitor& visitor) override {
        visitor.visit_array(*this);
    }

    std::unique_ptr<Expression> items;
};

class Identifier: public Expression {
  public:
    Identifier(location& loc, std::string id) :
        Expression(loc, Opcode::Identifier),
        id(std::move(id)) {}

    void accept(Visitor& visitor) override {
        visitor.visit_identifier(*this);
    }

    std::string id;
};

class BinaryExpression: public Expression {
  public:
    BinaryExpression(
        location& loc,
        std::unique_ptr<Expression> left,
        std::unique_ptr<Expression> right,
        const Opcode opcode
    ) :
        Expression(loc, opcode),
        left(std::move(left)),
        right(std::move(right)) {}

    void accept(Visitor& visitor) override {
        visitor.visit_binary_expression(*this);
    }

    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
};

class UnaryExpression: public Expression {
  public:
    UnaryExpression(
        location& loc,
        std::unique_ptr<Expression> left,
        const Opcode opcode
    ) :
        Expression(loc, opcode),
        left(std::move(left)) {}

    void accept(Visitor& visitor) override {
        visitor.visit_unary_expression(*this);
    }

    std::unique_ptr<Expression> left;
};

class Statement {
  public:
    Statement(location& loc) : line_number(loc.begin.line) {}

    virtual ~Statement() = default;
    virtual void accept(Visitor& visitor) = 0;
    int line_number;
};

class ExpressionStatement: public Statement {
  public:
    ExpressionStatement(location& loc, std::unique_ptr<Expression> expression) :
        Statement(loc),
        expression(std::move(expression)) {}

    void accept(Visitor& visitor) override {
        visitor.visit_expression_statement(*this);
    }

    std::unique_ptr<Expression> expression;
};

class WaitStatement: public Statement {
  public:
    WaitStatement(
        location& loc,
        std::vector<std::string> id_list,
        std::unique_ptr<Expression> expression
    ) :
        Statement(loc),
        id_list(std::move(id_list)),
        expression(std::move(expression)) {}

    void accept(Visitor& visitor) override {
        visitor.visit_wait_statement(*this);
    }

    std::vector<std::string> id_list;
    std::unique_ptr<Expression> expression;
};

class StatementList {
  public:
    StatementList() = default;

    StatementList(std::vector<std::unique_ptr<Statement>> statements) :
        inner(std::move(statements)) {}

    virtual ~StatementList() = default;
    std::vector<std::unique_ptr<Statement>> inner;

    void accept(Visitor& visitor) {
        visitor.visit_statement_list(*this);
    }
};

} // namespace dgeval::ast
