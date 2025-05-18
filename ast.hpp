#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>
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
    Plus = 13,
    Minus = 14,
    Star = 15,
    Slash = 16,
    Negate = 17,
    Not = 18,
    ArrayAccess = 19,
    Call = 20,
    Identifier = 23,
    Literal = 24,
};

const std::array<std::string, 25> mnemonic = {
    "nop", "comma", "assign", "cond", "alt", "band", "bor",  "eq",  "neq",
    "lt",  "lte",   "gt",     "gte",  "add", "sub",  "mul",  "div", "minus",
    "not", "aa",    "call",   "jmp",  "jf",  "id",   "const"
};

enum class Type : std::uint8_t { None, Number, String, Boolean, Array };

const std::array<std::string, 5> type_str =
    {"none", "number", "string", "boolean", "array"};

class TypeDescriptor {
  public:
    TypeDescriptor() = default;

    TypeDescriptor(Type type) : type(type) {}

    auto operator==(TypeDescriptor other) const -> bool {
        return type == other.type && dimension == other.dimension;
    }

    auto operator!=(TypeDescriptor other) const -> bool {
        return type != other.type || dimension != other.dimension;
    }

    auto is_array() const -> bool {
        // todo: remove array type check
        return dimension != 0 || type == Type::Array;
    }

    auto item_type() const -> TypeDescriptor {
        assert((is_array(), "can only call for array types"));

        TypeDescriptor type = *this;
        --type.dimension;

        return type;
    }

    auto to_string() const -> std::string {
        return type_str[std::to_underlying(type)];
    }

    Type type {};
    int dimension {};
};

class Expression {
  public:
    Expression() = default;
    virtual ~Expression() = default;

    Expression(int line_number, Type type) :
        line_number(line_number),
        opcode(Opcode::Literal),
        type_desc(TypeDescriptor(type)) {}

    Expression(int line, Opcode opcode) : line_number(line), opcode(opcode) {}

    virtual void accept(Visitor& visitor) = 0;

    int line_number {};
    Opcode opcode {};
    TypeDescriptor type_desc;
};

class NumberLiteral: public Expression {
  public:
    NumberLiteral(int line, double value) :
        Expression(line, Type::Number),
        value(value) {}

    void accept(Visitor& visitor) override {
        visitor.visit_number(*this);
    }

    double value;
};

class StringLiteral: public Expression {
  public:
    StringLiteral(int line, std::string value) :
        Expression(line, Type::String),
        value(std::move(value)) {}

    void accept(Visitor& visitor) override {
        visitor.visit_string(*this);
    }

    std::string value;
};

class BooleanLiteral: public Expression {
  public:
    BooleanLiteral(int line, bool value) :
        Expression(line, Type::Boolean),
        value(value) {}

    void accept(Visitor& visitor) override {
        visitor.visit_boolean(*this);
    }

    bool value;
};

class ArrayLiteral: public Expression {
  public:
    ArrayLiteral(int line, std::unique_ptr<Expression> items) :
        Expression(line, Type::Array),
        items(std::move(items)) {}

    void accept(Visitor& visitor) override {
        visitor.visit_array(*this);
    }

    std::unique_ptr<Expression> items;
};

class Identifier: public Expression {
  public:
    Identifier(int line, std::string id) :
        Expression(line, Opcode::Identifier),
        id(std::move(id)) {}

    void accept(Visitor& visitor) override {
        visitor.visit_identifier(*this);
    }

    std::string id;
};

class BinaryExpression: public Expression {
  public:
    BinaryExpression(
        int line_number,
        std::unique_ptr<Expression> left,
        std::unique_ptr<Expression> right,
        const Opcode opcode
    ) :
        Expression(line_number, opcode),
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
        int line,
        std::unique_ptr<Expression> left,
        const Opcode opcode
    ) :
        Expression(line, opcode),
        left(std::move(left)) {}

    void accept(Visitor& visitor) override {
        visitor.visit_unary_expression(*this);
    }

    std::unique_ptr<Expression> left;
};

class Statement {
  public:
    Statement(int line_number) : line_number(line_number) {}

    virtual ~Statement() = default;
    virtual void accept(Visitor& visitor) = 0;
    int line_number;
};

class ExpressionStatement: public Statement {
  public:
    ExpressionStatement(int line, std::unique_ptr<Expression> expression) :
        Statement(line),
        expression(std::move(expression)) {}

    void accept(Visitor& visitor) override {
        visitor.visit_expression_statement(*this);
    }

    std::unique_ptr<Expression> expression;
};

class WaitStatement: public Statement {
  public:
    WaitStatement(
        int line,
        std::vector<std::string> id_list,
        std::unique_ptr<Expression> expression
    ) :
        Statement(line),
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
