#pragma once

#include <map>
#include <memory>
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
    Jump = 21,
    JumpFalse = 22,
    JumpTrue = 23,
    Identifier = 24,
    Literal = 25,
    CallLRT = 26,
    Pop = 27,
};

const std::array<std::string, 28> MNEMONICS = {
    "nop", "comma", "assign", "cond",  "alt",   "band", "bor",
    "eq",  "neq",   "lt",     "lte",   "gt",    "gte",  "add",
    "sub", "mul",   "div",    "minus", "not",   "aa",   "call",
    "jmp", "jf",    "jt",     "id",    "const", "lrt",  "pop"
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
            return "(" + s + "," + " " + std::to_string(dimension) + ")";
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
    int idNdx;
    TypeDescriptor return_type;
    size_t parameter_count;
    std::vector<TypeDescriptor> parameters;
};

const std::map<std::string, FunctionSignature> RUNTIME_LIBRARY = {
    {"stddev", {0, NUMBER, 1, {{Type::Number, 1}}}},
    {"mean", {1, NUMBER, 1, {{Type::Number, 1}}}},
    {"count", {2, NUMBER, 1, {{Type::Number, 1}}}},
    {"min", {3, NUMBER, 1, {{Type::Number, 1}}}},
    {"max", {4, NUMBER, 1, {{Type::Number, 1}}}},
    {"sin", {5, NUMBER, 1, {NUMBER}}},
    {"cos", {6, NUMBER, 1, {NUMBER}}},
    {"tan", {7, NUMBER, 1, {NUMBER}}},
    {"pi", {8, NUMBER, 0}},
    {"atan", {9, NUMBER, 1, {NUMBER}}},
    {"asin", {10, NUMBER, 1, {NUMBER}}},
    {"acos", {11, NUMBER, 1, {NUMBER}}},
    {"exp", {12, NUMBER, 1, {NUMBER}}},
    {"ln", {13, NUMBER, 1, {NUMBER}}},
    {"print", {14, NUMBER, 1, {STRING}}},
    {"random", {15, NUMBER, 1, {NUMBER}}},
    {"len", {16, NUMBER, 1, {STRING}}},
    {"right", {17, NUMBER, 2, {STRING, NUMBER}}},
    {"left", {18, NUMBER, 2, {STRING, NUMBER}}},
};

struct SymbolDescriptor {
    TypeDescriptor type_desc;
    int idx {-1};
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

    virtual void accept(Visitor<void>& visitor) = 0;
    virtual auto accept(Visitor<std::unique_ptr<Expression>>& visitor)
        -> std::unique_ptr<Expression> = 0;

    location loc;
    Opcode opcode {};
    TypeDescriptor type_desc;
    int idNdx {-1};
    int stack_load {1};
    int function_call_count {0};
    int assignment_count {0};
};

class NumberLiteral: public Expression {
  public:
    NumberLiteral(location& loc, double value) :
        Expression(loc, Type::Number),
        value(value) {}

    void accept(Visitor<void>& visitor) override {
        visitor.visit_number(*this);
    }

    virtual auto accept(Visitor<std::unique_ptr<Expression>>& visitor)
        -> std::unique_ptr<Expression> override {
        return visitor.visit_number(*this);
    }

    double value;
};

class StringLiteral: public Expression {
  public:
    StringLiteral(location& loc, std::string value, std::string raw_value) :
        Expression(loc, Type::String),
        value(std::move(value)),
        raw_value(std::move(raw_value)) {}

    void accept(Visitor<void>& visitor) override {
        visitor.visit_string(*this);
    }

    virtual auto accept(Visitor<std::unique_ptr<Expression>>& visitor)
        -> std::unique_ptr<Expression> override {
        return visitor.visit_string(*this);
    }

    std::string value;
    std::string raw_value;
};

class BooleanLiteral: public Expression {
  public:
    BooleanLiteral(location& loc, bool value) :
        Expression(loc, Type::Boolean),
        value(value) {}

    void accept(Visitor<void>& visitor) override {
        visitor.visit_boolean(*this);
    }

    virtual auto accept(Visitor<std::unique_ptr<Expression>>& visitor)
        -> std::unique_ptr<Expression> override {
        return visitor.visit_boolean(*this);
    }

    bool value;
};

class ArrayLiteral: public Expression {
  public:
    ArrayLiteral(location& loc, std::unique_ptr<Expression> items) :
        Expression(loc, Type::Array),
        items(std::move(items)) {}

    void accept(Visitor<void>& visitor) override {
        visitor.visit_array(*this);
    }

    virtual auto accept(Visitor<std::unique_ptr<Expression>>& visitor)
        -> std::unique_ptr<Expression> override {
        return visitor.visit_array(*this);
    }

    std::unique_ptr<Expression> items;
    size_t item_count {};
};

class Identifier: public Expression {
  public:
    Identifier(location& loc, std::string id) :
        Expression(loc, Opcode::Identifier),
        id(std::move(id)) {}

    void accept(Visitor<void>& visitor) override {
        visitor.visit_identifier(*this);
    }

    virtual auto accept(Visitor<std::unique_ptr<Expression>>& visitor)
        -> std::unique_ptr<Expression> override {
        return visitor.visit_identifier(*this);
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

    void accept(Visitor<void>& visitor) override {
        visitor.visit_binary_expression(*this);
    }

    virtual auto accept(Visitor<std::unique_ptr<Expression>>& visitor)
        -> std::unique_ptr<Expression> override {
        return visitor.visit_binary_expression(*this);
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

    void accept(Visitor<void>& visitor) override {
        visitor.visit_unary_expression(*this);
    }

    virtual auto accept(Visitor<std::unique_ptr<Expression>>& visitor)
        -> std::unique_ptr<Expression> override {
        return visitor.visit_unary_expression(*this);
    }

    std::unique_ptr<Expression> left;
};

class Statement {
  public:
    Statement(location& loc, std::unique_ptr<Expression> expression) :
        line_number(loc.begin.line),
        expression(std::move(expression)) {}

    virtual ~Statement() = default;
    virtual void accept(Visitor<void>& visitor) = 0;
    virtual auto accept(Visitor<std::unique_ptr<Expression>>& visitor)
        -> std::unique_ptr<Expression> = 0;
    int line_number;
    std::unique_ptr<Expression> expression;
};

class ExpressionStatement: public Statement {
  public:
    ExpressionStatement(location& loc, std::unique_ptr<Expression> expression) :
        Statement(loc, std::move(expression)) {}

    void accept(Visitor<void>& visitor) override {
        visitor.visit_expression_statement(*this);
    }

    auto accept(Visitor<std::unique_ptr<Expression>>& visitor)
        -> std::unique_ptr<Expression> override {
        return visitor.visit_expression_statement(*this);
    }
};

class WaitStatement: public Statement {
  public:
    WaitStatement(
        location& loc,
        std::vector<std::string> id_list,
        std::unique_ptr<Expression> expression
    ) :
        Statement(loc, std::move(expression)),
        id_list(std::move(id_list)) {}

    void accept(Visitor<void>& visitor) override {
        visitor.visit_wait_statement(*this);
    }

    auto accept(Visitor<std::unique_ptr<Expression>>& visitor)
        -> std::unique_ptr<Expression> override {
        return visitor.visit_wait_statement(*this);
    }

    std::vector<std::string> id_list;
};

class StatementList {
  public:
    StatementList() = default;

    StatementList(std::vector<std::unique_ptr<Statement>> statements) :
        inner(std::move(statements)) {}

    virtual ~StatementList() = default;
    std::vector<std::unique_ptr<Statement>> inner;

    void accept(Visitor<void>& visitor) {
        visitor.visit_statement_list(*this);
    }

    auto accept(Visitor<std::unique_ptr<Expression>>& visitor)
        -> std::unique_ptr<Expression> {
        return visitor.visit_statement_list(*this);
    }
};

} // namespace dgeval::ast
