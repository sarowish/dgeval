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

class Visitor {
  public:
    virtual ~Visitor() = default;
    virtual void visit_program(Program& program) = 0;
    virtual void visit_statement_list(StatementList& statements) = 0;
    virtual void visit_expression_statement(ExpressionStatement& statement) = 0;
    virtual void visit_wait_statement(WaitStatement& statement) = 0;
    virtual void visit_expression(Expression& expression) = 0;
    virtual void visit_number(NumberLiteral& number) = 0;
    virtual void visit_string(StringLiteral& string) = 0;
    virtual void visit_boolean(BooleanLiteral& boolean) = 0;
    virtual void visit_array(ArrayLiteral& array) = 0;
    virtual void visit_identifier(Identifier& identifier) = 0;
    virtual void visit_binary_expression(BinaryExpression& expression) = 0;
    virtual void visit_unary_expression(UnaryExpression& expression) = 0;
};

} // namespace dgeval::ast
