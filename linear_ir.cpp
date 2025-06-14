#include "linear_ir.hpp"
#include "context.hpp"
#include "optimize.hpp"

namespace dgeval::ast {

LinearIR::LinearIR(OptimizationFlags flags) :
    skip_dead_statements(flags[Optimization::DeadStatement]),
    skip_dead_parts(flags[Optimization::DeadExpressionPart]) {}

void LinearIR::visit_program(Program& program) {
    program.statements->accept(*this);
    program.instructions = std::move(instructions);
}

void LinearIR::visit_statement_list(StatementList& statements) {
    for (auto& statement : statements.inner) {
        if (!skip_dead_statements || statement->expression->is_effective()) {
            statement->accept(*this);
            push_pop(statement->expression->stack_load);
        }
    }

    instructions.emplace_back(Opcode::CallLRT, 8);
    instructions.back().value = 0.0;
}

void LinearIR::visit_expression_statement(ExpressionStatement& statement) {
    statement.expression->accept(*this);
}

void LinearIR::visit_wait_statement(WaitStatement& statement) {
    statement.expression->accept(*this);
}

void LinearIR::visit_expression(Expression& expression) {}

void LinearIR::visit_number(NumberLiteral& number) {
    instructions.emplace_back(number.type_desc);
    instructions.back().value = number.value;
}

void LinearIR::visit_string(StringLiteral& string) {
    instructions.emplace_back(string);
    instructions.back().value = string.value;
}

void LinearIR::visit_boolean(BooleanLiteral& boolean) {
    instructions.emplace_back(boolean.type_desc);
    instructions.back().value = boolean.value;
}

void LinearIR::visit_array(ArrayLiteral& array) {
    switch_context(*array.items, true);

    instructions.emplace_back(array);
    instructions.back().value = static_cast<double>(array.item_count);
}

void LinearIR::visit_identifier(Identifier& identifier) {
    instructions.emplace_back(
        identifier.opcode,
        identifier.idNdx,
        identifier.type_desc
    );
    instructions.back().value = identifier.id;
}

void LinearIR::visit_binary_expression(BinaryExpression& binary_expr) {
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
        instructions.emplace_back(Opcode::JumpFalse, 0, NUMBER);

    } else if (binary_expr.opcode == Opcode::Alt) {
        instructions.emplace_back(Opcode::Jump, 0, NUMBER);
        instructions[start].parameter = instructions.size();
        start = instructions.size() - 1;
    }

    if (binary_expr.opcode == Opcode::Call) {
        if (right) {
            switch_context(*right, true);
        }
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

    if (right && right->opcode == Opcode::Comma
        && binary_expr.opcode != Opcode::Call) {
        push_pop(right->stack_load - 1);
        right->stack_load = 1;
    }

    switch (binary_expr.opcode) {
        case Opcode::Alt:
            instructions[start].parameter = instructions.size();
            return;
        case Opcode::Comma:
            binary_expr.stack_load = left->stack_load + right->stack_load;
            return;
        case Opcode::Conditional:
            return;
        default:
            break;
    }

    instructions.emplace_back(
        binary_expr.opcode,
        binary_expr.idNdx,
        binary_expr.type_desc
    );

    switch (binary_expr.opcode) {
        case Opcode::CallLRT: {
            auto& value = instructions.back().value;
            if (binary_expr.idNdx == 6 || binary_expr.idNdx == 7) {
                value = static_cast<double>(binary_expr.stack_load);
            } else if (std::holds_alternative<std::monostate>(value)) {
                value = 0.0;
            }
        } break;
        case Opcode::Assign: {
            auto const& id = dynamic_cast<Identifier*>(left.get())->id;
            instructions.back().value = id;
        } break;
        case Opcode::Call: {
            auto const& id = dynamic_cast<Identifier*>(left.get())->id;
            auto& instruction = instructions.back();
            instruction.value = id;
            instruction.parameter = RUNTIME_LIBRARY.at(id).parameter_count;
        } break;
        case Opcode::LessEqual:
        case Opcode::GreaterEqual:
        case Opcode::Less:
        case Opcode::Greater:
        case Opcode::NotEqual:
        case Opcode::Equal:
            instructions.back().type = binary_expr.left->type_desc;
        default:
            break;
    }
}

void LinearIR::visit_unary_expression(UnaryExpression& unary_expr) {
    unary_expr.left->accept(*this);

    instructions.emplace_back(
        unary_expr.opcode,
        unary_expr.idNdx,
        unary_expr.type_desc
    );

    if (unary_expr.opcode == Opcode::CallLRT) {
        instructions.back().value = 0.0;
    }
}

void LinearIR::push_pop(int count) {
    if (count != 0) {
        instructions.emplace_back(Opcode::Pop, count);
    }
}

void LinearIR::switch_context(Expression& expression, bool context) {
    bool temp = in_context;
    in_context = context;
    expression.accept(*this);
    in_context = temp;
}

} // namespace dgeval::ast
