#include "intermediate_code.hpp"
#include "context.hpp"
#include "optimize.hpp"

namespace dgeval::ast {

IntermediateCode::IntermediateCode(OptimizationFlags flags) :
    skip_dead_statements(flags[Optimization::DeadStatement]),
    skip_dead_parts(flags[Optimization::DeadExpressionPart]) {}

void IntermediateCode::visit_program(Program& program) {
    program.statements->accept(*this);
    program.instructions = std::move(instructions);
}

void IntermediateCode::visit_statement_list(StatementList& statements) {
    for (auto& statement : statements.inner) {
        if (!skip_dead_statements || statement->expression->is_effective()) {
            statement->accept(*this);
            push_pop(statement->expression->stack_load);
        }
    }

    instructions.emplace_back(Opcode::CallLRT, 8);
    instructions.back().value = 0.0;
}

void IntermediateCode::visit_expression_statement(ExpressionStatement& statement
) {
    statement.expression->accept(*this);
}

void IntermediateCode::visit_wait_statement(WaitStatement& statement) {
    statement.expression->accept(*this);
}

void IntermediateCode::visit_expression(Expression& expression) {}

void IntermediateCode::visit_number(NumberLiteral& number) {
    instructions.emplace_back(number.type_desc);
    instructions.back().value = number.value;
}

void IntermediateCode::visit_string(StringLiteral& string) {
    instructions.emplace_back(string);
    instructions.back().value = string.raw_value;
}

void IntermediateCode::visit_boolean(BooleanLiteral& boolean) {
    instructions.emplace_back(boolean.type_desc);
    instructions.back().value = boolean.value;
}

void IntermediateCode::visit_array(ArrayLiteral& array) {
    if (array.items) {
        switch_context(*array.items, true);
    }

    instructions.emplace_back(array);
    instructions.back().value = (double)array.item_count;
}

void IntermediateCode::visit_identifier(Identifier& identifier) {
    instructions.emplace_back(
        identifier.opcode,
        identifier.idNdx,
        identifier.type_desc
    );
    instructions.back().value = identifier.id;
}

void IntermediateCode::visit_binary_expression(BinaryExpression& binary_expr) {
    auto& left = binary_expr.left;
    auto& right = binary_expr.right;
    size_t start = instructions.size() - 1;

    if (binary_expr.opcode != Opcode::Assign
        && binary_expr.opcode != Opcode::Call) {
        left->accept(*this);
    }

    if (left->opcode == Opcode::Comma && binary_expr.opcode != Opcode::Comma) {
        push_pop(left->stack_load - 1);
        left->stack_load = 1;
    }

    if (binary_expr.opcode == Opcode::Conditional) {
        instructions.emplace_back(Opcode::JumpFalse, 0);

    } else if (binary_expr.opcode == Opcode::Alt) {
        instructions.emplace_back(Opcode::Jump, 0);
        instructions[start].parameter = instructions.size();
        start = instructions.size() - 1;
    }

    if (binary_expr.opcode == Opcode::Call) {
        switch_context(*right, true);
    } else if (binary_expr.opcode != Opcode::Comma || !skip_dead_parts
               || right->is_effective() || in_context) {
        if (right->opcode == Opcode::Comma) {
            switch_context(*right, false);
        } else {
            right->accept(*this);
        }
    } else {
        --right->stack_load;
    }

    if (right->opcode == Opcode::Comma && binary_expr.opcode != Opcode::Call) {
        push_pop(right->stack_load - 1);
        right->stack_load = 1;
    }

    switch (binary_expr.opcode) {
        case Opcode::Alt:
            instructions[start].parameter = instructions.size();
            break;
        case Opcode::Comma:
            binary_expr.stack_load = left->stack_load + right->stack_load;
            break;
        case Opcode::Conditional:
            break;
        default:
            instructions.emplace_back(
                binary_expr.opcode,
                binary_expr.idNdx,
                binary_expr.type_desc
            );

            if (binary_expr.opcode == Opcode::CallLRT
                && (binary_expr.idNdx == 6 || binary_expr.idNdx == 7)) {
                instructions.back().value = (double)binary_expr.stack_load;
            } else if (binary_expr.opcode == Opcode::Assign) {
                auto const& id = dynamic_cast<Identifier*>(left.get())->id;
                instructions.back().value = id;
            } else if (binary_expr.opcode == Opcode::Call) {
                auto const& id = dynamic_cast<Identifier*>(left.get())->id;
                auto& instruction = instructions.back();
                instruction.value = id;
                instruction.parameter = RUNTIME_LIBRARY.at(id).parameter_count;
            }
    }
}

void IntermediateCode::visit_unary_expression(UnaryExpression& unary_expr) {
    unary_expr.left->accept(*this);

    instructions.emplace_back(
        unary_expr.opcode,
        unary_expr.idNdx,
        unary_expr.type_desc
    );
}

void IntermediateCode::push_pop(int count) {
    if (count != 0) {
        instructions.emplace_back(Opcode::Pop, count);
    }
}

void IntermediateCode::switch_context(Expression& expression, bool context) {
    bool temp = in_context;
    in_context = context;
    expression.accept(*this);
    in_context = temp;
}

} // namespace dgeval::ast
