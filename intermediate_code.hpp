#pragma once

#include <variant>
#include "context.hpp"

namespace dgeval::ast {

class Instruction {
  public:
    Instruction(TypeDescriptor type) : type(type), opcode(Opcode::Literal) {}

    Instruction(Expression& expr) :
        opcode(expr.opcode),
        parameter(expr.idNdx),
        type(expr.type_desc) {}

    Instruction(Opcode opcode, int parameter) :
        opcode(opcode),
        parameter(parameter) {}

    Instruction(Opcode opcode, int parameter, TypeDescriptor type) :
        opcode(opcode),
        parameter(parameter != -1 ? parameter : 0),
        type(type) {}

    Opcode opcode {Opcode::None};
    int parameter {};
    int code_offset {};
    TypeDescriptor type;
    std::variant<std::monostate, double, std::string, bool> value;
};

class IntermediateCode: public Visitor<void> {
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
    void push_pop(int count);

    std::vector<Instruction> instructions;
};

} // namespace dgeval::ast
