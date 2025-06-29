#pragma once

#include "context.hpp"

namespace dgeval::ast {

class Fold: public Visitor<std::unique_ptr<Expression>> {
    std::vector<Message> errors;

  public:
    auto visit_program(Program& program)
        -> std::unique_ptr<Expression> override;
    auto visit_statement_list(StatementList& statements)
        -> std::unique_ptr<Expression> override;
    auto visit_expression_statement(ExpressionStatement& statement)
        -> std::unique_ptr<Expression> override;
    auto visit_wait_statement(WaitStatement& statement)
        -> std::unique_ptr<Expression> override;
    auto visit_expression(Expression& expression)
        -> std::unique_ptr<Expression> override;
    auto visit_number(NumberLiteral& number)
        -> std::unique_ptr<Expression> override;
    auto visit_string(StringLiteral& string)
        -> std::unique_ptr<Expression> override;
    auto visit_boolean(BooleanLiteral& boolean)
        -> std::unique_ptr<Expression> override;
    auto visit_array(ArrayLiteral& array)
        -> std::unique_ptr<Expression> override;
    auto visit_identifier(Identifier& identifier)
        -> std::unique_ptr<Expression> override;
    auto visit_binary_expression(BinaryExpression& binary_expr)
        -> std::unique_ptr<Expression> override;
    auto visit_unary_expression(UnaryExpression& unary_expr)
        -> std::unique_ptr<Expression> override;
};

auto reduce_addition(BinaryExpression& binary_expr)
    -> std::unique_ptr<Expression>;
auto reduce_subtraction(BinaryExpression& binary_expr)
    -> std::unique_ptr<Expression>;
auto reduce_multiplication(BinaryExpression& binary_expr)
    -> std::unique_ptr<Expression>;
auto reduce_division(
    BinaryExpression& binary_expr,
    std::vector<Message>& errors
) -> std::unique_ptr<Expression>;
auto reduce_logical(BinaryExpression& binary_expr)
    -> std::unique_ptr<Expression>;
auto reduce_comparison(BinaryExpression& binary_expr)
    -> std::unique_ptr<Expression>;
auto reduce_ternary(BinaryExpression& binary_expr)
    -> std::unique_ptr<Expression>;

} // namespace dgeval::ast
