#pragma once

#include <fstream>
#include "context.hpp"

namespace dgeval::ast {

class Printer: public Visitor {
  public:
    Printer(const std::string& file_name) : output(file_name + ".json") {}

    void visit_program(Program& program) override;
    void visit_statement_list(StatementList& statements) override;
    void visit_expression_statement(ExpressionStatement& statement) override;
    void visit_wait_statement(WaitStatement& statement) override;
    void visit_expression(Expression& expression) override;
    void visit_number(NumberLiteral& number) override;
    void visit_string(StringLiteral& string) override;
    void visit_boolean(BooleanLiteral& boolean) override;
    void visit_array(ArrayLiteral& array) override;
    void visit_identifier(Identifier& identifier) override;
    void visit_binary_expression(BinaryExpression& binary_expr) override;
    void visit_unary_expression(UnaryExpression& unary_expr) override;

    std::ofstream output;
};

void join_strings(
    std::ofstream& output,
    std::vector<std::string>& strings,
    const std::string& delimiter
);

} // namespace dgeval::ast
