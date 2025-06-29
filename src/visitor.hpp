#pragma once

namespace dgeval::ast {

class Program;
class StatementList;
class ExpressionStatement;
class WaitStatement;
class Expression;
class NumberLiteral;
class StringLiteral;
class BooleanLiteral;
class ArrayLiteral;
class Identifier;
class BinaryExpression;
class UnaryExpression;

template<typename T = void>
class Visitor {
  public:
    virtual ~Visitor() = default;
    virtual auto visit_program(Program& program) -> T = 0;
    virtual auto visit_statement_list(StatementList& statements) -> T = 0;
    virtual auto visit_expression_statement(ExpressionStatement& statement)
        -> T = 0;
    virtual auto visit_wait_statement(WaitStatement& statement) -> T = 0;
    virtual auto visit_expression(Expression& expression) -> T = 0;
    virtual auto visit_number(NumberLiteral& number) -> T = 0;
    virtual auto visit_string(StringLiteral& string) -> T = 0;
    virtual auto visit_boolean(BooleanLiteral& boolean) -> T = 0;
    virtual auto visit_array(ArrayLiteral& array) -> T = 0;
    virtual auto visit_identifier(Identifier& identifier) -> T = 0;
    virtual auto visit_binary_expression(BinaryExpression& expression) -> T = 0;
    virtual auto visit_unary_expression(UnaryExpression& expression) -> T = 0;
};

} // namespace dgeval::ast
