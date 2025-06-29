#include "checker.hpp"
#include <format>
#include "ast.hpp"

namespace dgeval::ast {

void Checker::visit_program(Program& program) {
    symbol_table = std::move(program.symbol_table);
    program.statements->accept(*this);

    for (auto const& statement : program.circular_statements->inner) {
        errors.emplace_back(
            statement->line_number,
            "Statement is in circular dependency"
        );
    }

    program.symbol_table = std::move(symbol_table);
    program.messages = std::move(errors);
}

void Checker::visit_statement_list(StatementList& statements) {
    for (auto const& statement : statements.inner) {
        statement->accept(*this);
    }
}

void Checker::visit_expression_statement(ExpressionStatement& statement) {
    statement.expression->accept(*this);
}

void Checker::visit_wait_statement(WaitStatement& statement) {
    statement.expression->accept(*this);

    for (auto const& id : statement.id_list) {
        if (!symbol_table.contains(id)) {
            errors.emplace_back(
                statement.line_number,
                std::format("The symbol `{}` is not defined", id)
            );
        }
    }
}

void Checker::visit_expression(Expression& expression) {}

void Checker::visit_number(NumberLiteral& number) {}

void Checker::visit_string(StringLiteral& string) {}

void Checker::visit_boolean(BooleanLiteral& boolean) {}

void Checker::visit_array(ArrayLiteral& array) {
    expression_part_types.emplace();
    array.items->accept(*this);
    array.offload_count(*array.items);
    auto types = expression_part_types.top();
    if (!std::ranges::all_of(types, [&](TypeDescriptor type) {
            return type == array.items->type_desc;
        })) {
        errors.emplace_back(
            array.loc,
            "All items of an array should be of the same type"
        );
    }
    array.type_desc = array.items->type_desc;
    array.item_count = types.size() + 1;
    expression_part_types.pop();

    ++array.type_desc.dimension;
}

void Checker::visit_identifier(Identifier& identifier) {
    if (symbol_table.contains(identifier.id)) {
        auto const& symbol = symbol_table[identifier.id];
        identifier.type_desc = symbol.type_desc;
        if (opcode != Opcode::Assign) {
            identifier.idNdx = symbol.idx;
        }
    } else if (RUNTIME_LIBRARY.contains(identifier.id)) {
        if (opcode == Opcode::Call) {
            auto const& function_signature = RUNTIME_LIBRARY.at(identifier.id);
            identifier.type_desc = function_signature.return_type;
            identifier.idNdx = function_signature.idNdx;
        } else if (opcode != Opcode::Assign) {
            errors.emplace_back(
                identifier.loc,
                "Can't use runtime library function without calling it"
            );
        }
    } else {
        errors.emplace_back(
            identifier.loc,
            std::format("The variable `{}` is not defined", identifier.id)
        );
    }
}

void Checker::visit_binary_expression(BinaryExpression& binary_expr) {
    auto& left = binary_expr.left;
    auto& right = binary_expr.right;

    opcode = binary_expr.opcode;
    left->accept(*this);
    binary_expr.offload_count(*left);
    opcode = Opcode::None;

    if (right) {
        if ((binary_expr.opcode == Opcode::Call && left->type_desc != NONE)
            || right->opcode == Opcode::Comma) {
            expression_part_types.emplace();
        }
        right->accept(*this);
        binary_expr.offload_count(*right);
    }

    if (binary_expr.opcode != Opcode::Assign
            && binary_expr.opcode != Opcode::Comma && left->type_desc == NONE
        || binary_expr.opcode != Opcode::Conditional
            && binary_expr.opcode != Opcode::Call
            && binary_expr.opcode != Opcode::ArrayAccess
            && binary_expr.opcode != Opcode::Comma && right
            && right->type_desc == NONE) {
        return;
    }

    switch (binary_expr.opcode) {
        case Opcode::Assign:
            if (left->opcode == Opcode::Identifier) {
                auto const& id = dynamic_cast<Identifier*>(left.get())->id;
                if (RUNTIME_LIBRARY.contains(id)) {
                    errors.emplace_back(
                        binary_expr.loc,
                        std::format(
                            "Cannot redefine runtime library function name `{}` as a variable name",
                            id
                        )
                    );
                } else if (symbol_table[id].type_desc != NONE) {
                    errors.emplace_back(
                        binary_expr.loc,
                        std::format(
                            "The variable `{}` has already been defined",
                            id
                        )
                    );
                } else {
                    binary_expr.type_desc = right->type_desc;
                    binary_expr.left->type_desc = right->type_desc;
                    symbol_table[id].type_desc = binary_expr.type_desc;
                    binary_expr.idNdx = symbol_table[id].idx;
                    binary_expr.assignment_count++;
                }
            } else {
                errors.emplace_back(
                    binary_expr.loc,
                    "The LHS of the assignment operator must be an identifier"
                );
            }
            break;
        case Opcode::Conditional:
            if (left->type_desc != BOOLEAN) {
                errors.emplace_back(
                    binary_expr.loc,
                    "The first operand of the ternary operator should be `bool`"
                );
            }

            binary_expr.type_desc = right->type_desc;
            break;
        case Opcode::Alt:
            if (left->type_desc != right->type_desc) {
                errors.emplace_back(
                    binary_expr.loc,
                    "Last 2 operands of the ternary operator should be of the same type"
                );
            } else {
                binary_expr.type_desc = left->type_desc;
            }
            break;
        case Opcode::And:
        case Opcode::Or:
            if (left->type_desc != BOOLEAN || right->type_desc != BOOLEAN) {
                errors.emplace_back(
                    binary_expr.loc,
                    "Boolean operators can only be applied to `boolean` types"
                );
                break;
            }
            binary_expr.type_desc = BOOLEAN;
            break;
        case Opcode::Multiply:
        case Opcode::Divide:
        case Opcode::Subtract:
            if (left->type_desc != NUMBER || right->type_desc != NUMBER) {
                errors.emplace_back(
                    binary_expr.loc,
                    std::format(
                        "Operator `{}` requires its operands to be of the type `number`",
                        OPERATOR_SYMBOLS[std::to_underlying(binary_expr.opcode)]
                    )
                );
            } else {
                binary_expr.type_desc = NUMBER;
            }
            break;
        case Opcode::Add:
            if (left->type_desc == NUMBER && right->type_desc == NUMBER) {
                binary_expr.type_desc = NUMBER;
            } else if (left->type_desc == STRING
                           && (right->type_desc == STRING
                               || right->type_desc == NUMBER)
                       || left->type_desc == NUMBER
                           && right->type_desc == STRING) {
                binary_expr.type_desc = STRING;
            } else if (left->type_desc.is_array()) {
                if (left->type_desc.item_type() == right->type_desc) {
                    binary_expr.type_desc = left->type_desc;
                } else {
                    errors.emplace_back(
                        binary_expr.loc,
                        "The item being appended should be the same type as the array's items"
                    );
                }
            } else {
                errors.emplace_back(
                    binary_expr.loc,
                    std::format(
                        "Cannot add `{}` to `{}`",
                        right->type_desc.to_string(),
                        left->type_desc.to_string()
                    )
                );
            }
            break;
        case Opcode::Less:
        case Opcode::LessEqual:
        case Opcode::Greater:
        case Opcode::GreaterEqual:
            if (left->type_desc.is_array() || left->type_desc == BOOLEAN) {
                errors.emplace_back(
                    binary_expr.loc,
                    std::format(
                        "Operator `{}` is not supported for `{}`",
                        OPERATOR_SYMBOLS[std::to_underlying(binary_expr
                                                                .opcode)],
                        left->type_desc.to_string()
                    )
                );
            } else if (left->type_desc != right->type_desc) {
                errors.emplace_back(
                    binary_expr.loc,
                    std::format(
                        "Operator `{}` requires its operands to be of the same type",
                        OPERATOR_SYMBOLS[std::to_underlying(binary_expr.opcode)]
                    )
                );
            } else {
                binary_expr.type_desc = BOOLEAN;
            }
            break;
        case Opcode::Equal:
        case Opcode::NotEqual:
            if (left->type_desc != right->type_desc) {
                errors.emplace_back(
                    binary_expr.loc,
                    std::format(
                        "Operator `{}` requires its operands to be of the same type",
                        OPERATOR_SYMBOLS[std::to_underlying(binary_expr.opcode)]
                    )
                );
            } else {
                binary_expr.type_desc = BOOLEAN;
            }
            break;
        case Opcode::ArrayAccess:
            if (!left->type_desc.is_array()) {
                errors.emplace_back(
                    binary_expr.loc,
                    "Array access operator can only be applied to an array"
                );
            } else if (right->opcode == Opcode::Comma) {
                errors.emplace_back(
                    binary_expr.loc,
                    "Cannot index an array by a list of expressions"

                );
            } else if (right->type_desc != NUMBER) {
                errors.emplace_back(
                    binary_expr.loc,
                    "Array index should be `number`"
                );
            } else {
                binary_expr.type_desc.type = left->type_desc.type;
                binary_expr.type_desc.dimension = left->type_desc.dimension - 1;
            }

            break;
        case Opcode::Call:
            if (left->opcode != Opcode::Identifier) {
                errors.emplace_back(
                    binary_expr.loc,
                    "The first operand of a call operator can only be an identifier"
                );
            } else {
                auto const& func_id = dynamic_cast<Identifier*>(left.get())->id;
                auto const& function_signature = RUNTIME_LIBRARY.at(func_id);
                size_t argument_count = 0;
                if (right) {
                    auto& argument_types = expression_part_types.top();
                    argument_types.insert(
                        argument_types.begin(),
                        right->type_desc
                    );
                    argument_count = argument_types.size();
                }
                if (function_signature.parameter_count != argument_count) {
                    errors.emplace_back(
                        binary_expr.loc,
                        std::format(
                            "Mismatch in function argument count: expected {}, received {}",
                            function_signature.parameter_count,
                            argument_count
                        )
                    );
                }
                for (size_t idx = 0; idx
                     < std::min(function_signature.parameter_count,
                                argument_count);
                     ++idx) {
                    auto const& argument_types = expression_part_types.top();
                    auto const& parameter = function_signature.parameters[idx];
                    auto const& argument = argument_types[idx];
                    if (argument != parameter && argument != NONE) {
                        errors.emplace_back(
                            binary_expr.loc,
                            std::format(
                                "Type mismatch in function argument position {}: expected `{}`, received `{}`",
                                idx + 1,
                                parameter.to_string(),
                                argument.to_string()
                            )
                        );
                    }
                }
                binary_expr.type_desc = left->type_desc;
                binary_expr.function_call_count++;
            }
            if (right) {
                expression_part_types.pop();
            }
            break;
        case Opcode::Comma:
            binary_expr.type_desc = left->type_desc;
            // binary_expr.stack_load += left->stack_load;

            if (right->opcode == Opcode::Comma) {
                expression_part_types.pop();
            }

            if (!expression_part_types.empty()) {
                expression_part_types.top().push_back(right->type_desc);
            }

            break;
        default:
            break;
    }
}

void Checker::visit_unary_expression(UnaryExpression& unary_expr) {
    auto& left = unary_expr.left;

    left->accept(*this);
    unary_expr.offload_count(*left);

    if (left->type_desc.type == Type::None) {
        return;
    }

    switch (unary_expr.opcode) {
        case Opcode::Not:
            if (left->type_desc != BOOLEAN) {
                errors.emplace_back(
                    unary_expr.loc,
                    "Unary `!` operator requires its operand to be of type `boolean`"
                );
                break;
            }

            unary_expr.type_desc = left->type_desc;
            break;
        case Opcode::Minus:
            if (left->type_desc != NUMBER) {
                errors.emplace_back(
                    unary_expr.loc,
                    "Unary `-` operator requires its operand to be of type `number`"
                );
                break;
            }

            unary_expr.type_desc = left->type_desc;
            break;
        default:
            break;
    }
}

} // namespace dgeval::ast
