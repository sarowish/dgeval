#include "checker.hpp"
#include <algorithm>
#include <format>
#include <print>
#include <utility>
#include "ast.hpp"
#include "context.hpp"

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

    program.symbol_table = symbol_table;
    program.messages = errors;
    program.messages.emplace_back("Completed compilation");
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

    for (auto id : statement.id_list) {
        if (!symbol_table.contains(id)) {
            errors.emplace_back(
                statement.line_number,
                std::format("The variable `{}` is not defined", id)
            );
        }
    }
}

void Checker::visit_expression(Expression& expression) {}

void Checker::visit_number(NumberLiteral& number) {}

void Checker::visit_string(StringLiteral& string) {}

void Checker::visit_boolean(BooleanLiteral& boolean) {}

void Checker::visit_array(ArrayLiteral& array) {
    if (array.items) {
        list_item_types.emplace();
        array.items->accept(*this);
        auto types = list_item_types.top();
        if (!std::ranges::all_of(types, [&](TypeDescriptor type) {
                return type == array.items->type_desc;
            })) {
            errors.emplace_back(
                array.loc,
                "All items of an array should be of the same type"
            );
        }
        list_item_types.pop();
        array.type_desc = array.items->type_desc;
    }
    ++array.type_desc.dimension;
}

void Checker::visit_identifier(Identifier& identifier) {
    if (symbol_table.contains(identifier.id)) {
        identifier.type_desc = symbol_table[identifier.id];
    } else if (runtime_library.contains(identifier.id)) {
        identifier.type_desc = runtime_library.at(identifier.id).return_type;
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

    left->accept(*this);

    if (right) {
        if (binary_expr.opcode == Opcode::Call) {
            list_item_types.emplace();
        }
        right->accept(*this);
    }

    if (binary_expr.opcode != Opcode::Assign
            && left->type_desc.type == Type::None
        || right && right->type_desc.type == Type::None) {
        return;
    }

    switch (binary_expr.opcode) {
        case Opcode::Assign:
            if (left->opcode == Opcode::Identifier) {
                auto const& id = dynamic_cast<Identifier*>(left.get())->id;
                if (runtime_library.contains(id)) {
                    errors.emplace_back(
                        binary_expr.loc,
                        std::format(
                            "Cannot redefine runtime library function name `{}` as a variable name",
                            id
                        )
                    );
                } else if (symbol_table[id] != TypeDescriptor()) {
                    errors.emplace_back(
                        binary_expr.loc,
                        std::format(
                            "The variable `{}` has already been defined",
                            id
                        )
                    );
                } else {
                    binary_expr.type_desc = right->type_desc;
                    symbol_table[id] = binary_expr.type_desc;
                }
            } else {
                errors.emplace_back(
                    binary_expr.loc,
                    std::format(
                        "cannot assign to an rvalue",
                        mnemonic[std::to_underlying(left->opcode)]
                    )
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
                    std::format(
                        "Last 2 operands of the ternary operator should be of the same type",
                        operator_symbol[std::to_underlying(binary_expr.opcode)]
                    )
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
        case Opcode::Star:
        case Opcode::Slash:
        case Opcode::Minus:
            if (left->type_desc != NUMBER || right->type_desc != NUMBER) {
                errors.emplace_back(
                    binary_expr.loc,
                    std::format(
                        "Operator `{}` requires its operands to be of type `number`",
                        operator_symbol[std::to_underlying(binary_expr.opcode)]
                    )
                );
            } else {
                binary_expr.type_desc = NUMBER;
            }
            break;
        case Opcode::Plus:
            if (left->type_desc == NUMBER && right->type_desc == NUMBER) {
                binary_expr.type_desc = NUMBER;
            } else if (left->type_desc == STRING
                           && (right->type_desc == STRING
                               || right->type_desc == NUMBER)
                       || left->type_desc == NUMBER
                           && right->type_desc == STRING) {
                binary_expr.type_desc = STRING;
            } else if (left->type_desc.is_array()
                       && left->type_desc.item_type() == right->type_desc) {
                binary_expr.type_desc = left->type_desc;
            } else {
                //todo: fix this message for arrays
                errors.emplace_back(
                    binary_expr.loc,
                    std::format(
                        "cannot add a `{}` to a `{}`",
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
                        operator_symbol[std::to_underlying(binary_expr.opcode)],
                        left->type_desc.to_string()
                    )
                );
            } else if (left->type_desc != right->type_desc) {
                errors.emplace_back(
                    binary_expr.loc,
                    std::format(
                        "Operator `{}` requires its operands to be of the same type",
                        operator_symbol[std::to_underlying(binary_expr.opcode)]
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
                        operator_symbol[std::to_underlying(binary_expr.opcode)]
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
                    std::format(
                        "cannot index a `{}`",
                        left->type_desc.to_string()
                    )
                );
            } else if (right->opcode == Opcode::Comma) {
                errors.emplace_back(
                    binary_expr.loc,
                    "cannot index an array by a list of expressions"

                );
            } else if (right->type_desc != NUMBER) {
                errors.emplace_back(
                    binary_expr.loc,
                    std::format(
                        "cannot index an array by a `{}`",
                        right->type_desc.to_string()
                    )
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
                    std::format(
                        "cannot call a `{}`",
                        left->type_desc.to_string()
                    )
                );
            } else {
                auto const& func_id = dynamic_cast<Identifier*>(left.get())->id;
                auto const& function_signature = runtime_library.at(func_id);
                size_t argument_count = 0;
                if (right) {
                    list_item_types.top().push_back(right->type_desc);
                    argument_count = list_item_types.top().size();
                }
                if (function_signature.parameter_count != argument_count) {
                    errors.emplace_back(
                        binary_expr.loc,
                        std::format(
                            "Mismatch in function argument count: expected `{}`, received `{}`",
                            function_signature.parameter_count,
                            argument_count
                        )
                    );
                    break;
                }
                for (size_t idx = 0; idx < function_signature.parameter_count;
                     ++idx) {
                    auto const& parameter = function_signature.parameters[idx];
                    auto const& argument = list_item_types.top()[idx];
                    if (argument != parameter) {
                        errors.emplace_back(
                            binary_expr.loc,
                            std::format(
                                "Mismatch in function argument types: expected `{}`, received `{}`",
                                parameter.to_string(),
                                argument.to_string()
                            )
                        );
                        list_item_types.pop();
                        return;
                    }
                }
                binary_expr.type_desc = left->type_desc;
            }
            if (right) {
                list_item_types.pop();
            }
            break;
        case Opcode::Comma:
            binary_expr.type_desc = right->type_desc;

            if (!list_item_types.empty()) {
                list_item_types.top().push_back(left->type_desc);
            }

            break;
        default:
            std::unreachable();
    }
}

void Checker::visit_unary_expression(UnaryExpression& unary_expr) {
    auto& left = unary_expr.left;

    left->accept(*this);

    if (left->type_desc.type == Type::None) {
        return;
    }

    switch (unary_expr.opcode) {
        case Opcode::Not:
            if (left->type_desc != TypeDescriptor(Type::Boolean)) {
                errors.emplace_back(
                    unary_expr.loc,
                    std::format(
                        "cannot apply unary `!` operator to `{}`",
                        left->type_desc.to_string()
                    )
                );
                break;
            }

            unary_expr.type_desc = left->type_desc;
            break;
        case Opcode::Negate:
            if (left->type_desc != TypeDescriptor(Type::Number)) {
                errors.emplace_back(
                    unary_expr.loc,
                    std::format(
                        "cannot apply unary `-` operator to `{}`",
                        left->type_desc.to_string()
                    )
                );
                break;
            }

            unary_expr.type_desc = left->type_desc;
            break;
        default:
            std::unreachable();
    }
}

} // namespace dgeval::ast
