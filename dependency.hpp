#pragma once

#include <unordered_set>
#include "context.hpp"

namespace dgeval::ast {

struct Relations {
    std::unordered_set<size_t> depends;
    std::unordered_set<size_t> defines;
};

class Dependency: public Visitor {
  public:
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

    Opcode opcode;
    size_t statement_idx;
    std::unordered_map<std::string, Relations> symbols;
};

} // namespace dgeval::ast
