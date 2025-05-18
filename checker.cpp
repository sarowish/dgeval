#include "checker.hpp"
#include <format>
#include <utility>
#include "ast.hpp"
#include "context.hpp"

namespace dgeval::ast {

const TypeDescriptor NUMBER(Type::Number);
const TypeDescriptor STRING(Type::String);
const TypeDescriptor BOOLEAN(Type::Boolean);

void Checker::visit_program(Program& program) {
    program.statements->accept(*this);
    program.symbol_table = symbol_table;
    program.messages = errors;
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
}

void Checker::visit_expression(Expression& expression) {}

void Checker::visit_number(NumberLiteral& number) {}

void Checker::visit_string(StringLiteral& string) {}

void Checker::visit_boolean(BooleanLiteral& boolean) {}

void Checker::visit_array(ArrayLiteral& array) {
    // todo: check types
    array.items->accept(*this);
    array.type_desc = array.items->type_desc;
    ++array.type_desc.dimension;
}

void Checker::visit_identifier(Identifier& identifier) {
    if (symbol_table.contains(identifier.id)) {
        identifier.type_desc = symbol_table[identifier.id];
    }
}

void Checker::visit_binary_expression(BinaryExpression& binary_expr) {
    auto& left = binary_expr.left;
    auto& right = binary_expr.right;

    left->accept(*this);
    right->accept(*this);

    if (binary_expr.opcode != Opcode::Assign
            && left->type_desc.type == Type::None
        || right->type_desc.type == Type::None) {
        return;
    }

    switch (binary_expr.opcode) {
        case Opcode::Assign:
            if (left->opcode == Opcode::Identifier) {
                auto const& id = dynamic_cast<Identifier*>(left.get())->id;
                if (symbol_table.contains(id)) {
                    errors.emplace_back(
                        binary_expr.line_number,
                        std::format(
                            "The variable `{}` has already been defined",
                            id
                        )
                    );
                    break;
                }
                binary_expr.type_desc = right->type_desc;
                symbol_table[id] = binary_expr.type_desc;
            } else {
                errors.emplace_back(
                    binary_expr.line_number,
                    std::format(
                        "cannot assign to a ``",
                        mnemonic[std::to_underlying(left->opcode)]
                    )
                );
            }
            break;
        case Opcode::Conditional:
            if (left->type_desc == BOOLEAN) {
                errors.emplace_back(
                    binary_expr.line_number,
                    "The first operand of the ternary operator should be `bool`"
                );
            }

            binary_expr.type_desc = right->type_desc;
            break;
        case Opcode::And:
        case Opcode::Or:
            if (left->type_desc != BOOLEAN || right->type_desc != BOOLEAN) {
                errors.emplace_back(
                    binary_expr.line_number,
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
                std::string text;

                switch (binary_expr.opcode) {
                    case Opcode::Star:
                        text = std::format(
                            "cannot multiply a `{}` by a `{}`",
                            left->type_desc.to_string(),
                            right->type_desc.to_string()
                        );
                        break;
                    case Opcode::Slash:
                        text = std::format(
                            "cannot divide a `{}` by a `{}`",
                            left->type_desc.to_string(),
                            right->type_desc.to_string()
                        );
                        break;
                    case Opcode::Minus:
                        text = std::format(
                            "cannot subtract a `{}` from a `{}`",
                            left->type_desc.to_string(),
                            right->type_desc.to_string()
                        );
                        break;
                    default:
                }

                errors.emplace_back(binary_expr.line_number, text);
                break;
            }
            binary_expr.type_desc = NUMBER;
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
                    binary_expr.line_number,
                    std::format(
                        "cannot add a `{}` to a `{}`",
                        right->type_desc.to_string(),
                        left->type_desc.to_string()
                    )
                );
            }
            break;
        case Opcode::ArrayAccess:
            if (!left->type_desc.is_array()) {
                errors.emplace_back(
                    binary_expr.line_number,
                    std::format(
                        "cannot index a `{}`",
                        left->type_desc.to_string()
                    )
                );
            } else if (right->type_desc != NUMBER) {
                errors.emplace_back(
                    binary_expr.line_number,
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
                    binary_expr.line_number,
                    std::format(
                        "cannot call a `{}`",
                        left->type_desc.to_string()
                    )
                );
            }
            break;
        case Opcode::Comma:
            binary_expr.type_desc = right->type_desc;
            break;
        default:
            std::unreachable();
    }
}

void Checker::visit_unary_expression(UnaryExpression& unary_expr) {
    auto& left = unary_expr.left;

    switch (unary_expr.opcode) {
        case Opcode::Not:
            if (left->type_desc != TypeDescriptor(Type::Boolean)) {
                errors.emplace_back(
                    unary_expr.line_number,
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
                    unary_expr.line_number,
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
