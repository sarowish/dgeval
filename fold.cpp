#include "fold.hpp"
#include "ast.hpp"

namespace dgeval::ast {

auto Fold::visit_program(Program& program) -> std::unique_ptr<Expression> {
    return program.statements->accept(*this);
}

auto Fold::visit_statement_list(StatementList& statements)
    -> std::unique_ptr<Expression> {
    for (auto& statement : statements.inner) {
        statement->accept(*this);
    }

    return nullptr;
}

auto Fold::visit_expression_statement(ExpressionStatement& statement)
    -> std::unique_ptr<Expression> {
    if (auto r = statement.expression->accept(*this)) {
        statement.expression = std::move(r);
    }

    return nullptr;
}

auto Fold::visit_wait_statement(WaitStatement& statement)
    -> std::unique_ptr<Expression> {
    if (auto r = statement.expression->accept(*this)) {
        statement.expression = std::move(r);
    }

    return nullptr;
}

auto Fold::visit_expression(Expression& expression)
    -> std::unique_ptr<Expression> {
    return nullptr;
}

auto Fold::visit_number(NumberLiteral& number) -> std::unique_ptr<Expression> {
    return nullptr;
}

auto Fold::visit_string(StringLiteral& string) -> std::unique_ptr<Expression> {
    string.opcode = Opcode::CallLRT;
    string.idNdx = 3;
    return nullptr;
}

auto Fold::visit_boolean(BooleanLiteral& boolean)
    -> std::unique_ptr<Expression> {
    return nullptr;
}

auto Fold::visit_array(ArrayLiteral& array) -> std::unique_ptr<Expression> {
    if (array.items) {
        if (auto r = array.items->accept(*this)) {
            array.items = std::move(r);
        }
    }

    array.opcode = Opcode::CallLRT;
    array.idNdx = 0;
    --array.type_desc.dimension;

    return nullptr;
}

auto Fold::visit_identifier(Identifier& identifier)
    -> std::unique_ptr<Expression> {
    return nullptr;
}

auto Fold::visit_binary_expression(BinaryExpression& binary_expr)
    -> std::unique_ptr<Expression> {
    auto& left = binary_expr.left;
    auto& right = binary_expr.right;
    int comparison_parameter = 0;

    if (auto l = binary_expr.left->accept(*this)) {
        binary_expr.left = std::move(l);
    }

    if (right) {
        if (auto r = binary_expr.right->accept(*this)) {
            binary_expr.right = std::move(r);
        }
    }

    switch (binary_expr.opcode) {
        case Opcode::Add:
            if (auto result = reduce_addition(binary_expr)) {
                if (result->type_desc == STRING) {
                    result->accept(*this);
                }
                return result;
            } else if (left->type_desc == STRING
                       && right->type_desc == STRING) {
                binary_expr.opcode = Opcode::CallLRT;
                binary_expr.idNdx = 4;
            } else if (left->type_desc.dimension != 0) {
                binary_expr.opcode = Opcode::CallLRT;
                binary_expr.idNdx = 2;
            }
            break;
        case Opcode::Subtract:
            return reduce_subtraction(binary_expr);
        case Opcode::Multiply:
            return reduce_multiplication(binary_expr);
        case Opcode::Divide:
            return reduce_division(binary_expr);
        case Opcode::And:
        case Opcode::Or:
            return reduce_logical(binary_expr);
        case Opcode::LessEqual:
            comparison_parameter++;
        case Opcode::GreaterEqual:
            comparison_parameter++;
        case Opcode::Less:
            comparison_parameter++;
        case Opcode::Greater:
            comparison_parameter++;
        case Opcode::NotEqual:
            comparison_parameter++;
        case Opcode::Equal:
            if (auto result = reduce_comparison(binary_expr)) {
                return result;
            } else if (left->type_desc == STRING) {
                binary_expr.opcode = Opcode::CallLRT;
                binary_expr.idNdx = 6;
                binary_expr.stack_load = comparison_parameter;
            } else if (left->type_desc.dimension != 0) {
                binary_expr.opcode = Opcode::CallLRT;
                binary_expr.idNdx = 7;
                binary_expr.stack_load = comparison_parameter;
            }
            break;
        case Opcode::Conditional:
            return reduce_ternary(binary_expr);
        case Opcode::ArrayAccess:
            binary_expr.opcode = Opcode::CallLRT;
            binary_expr.idNdx = 1;
            break;
        default:
            break;
    }

    return nullptr;
}

auto Fold::visit_unary_expression(UnaryExpression& unary_expr)
    -> std::unique_ptr<Expression> {
    if (auto r = unary_expr.left->accept(*this)) {
        unary_expr.left = std::move(r);
    }

    switch (unary_expr.opcode) {
        case Opcode::Not:
            if (auto const& boolean =
                    dynamic_cast<BooleanLiteral*>(unary_expr.left.get())) {
                return std::make_unique<NumberLiteral>(
                    unary_expr.loc,
                    !boolean->value
                );
            }
            break;
        case Opcode::Minus:
            if (auto const& number =
                    dynamic_cast<NumberLiteral*>(unary_expr.left.get())) {
                return std::make_unique<NumberLiteral>(
                    unary_expr.loc,
                    -number->value
                );
            }
            break;
        default:
            break;
    }

    return nullptr;
}

auto reduce_ternary(BinaryExpression& binary_expr)
    -> std::unique_ptr<Expression> {
    auto const& ln = dynamic_cast<BooleanLiteral*>(binary_expr.left.get());
    auto const& rn = dynamic_cast<BinaryExpression*>(binary_expr.right.get());

    if (ln) {
        return std::move(ln->value ? rn->left : rn->right);
    }

    return nullptr;
}

auto reduce_comparison(BinaryExpression& binary_expr)
    -> std::unique_ptr<Expression> {
    if (binary_expr.left->opcode != Opcode::Literal
        || binary_expr.right->opcode != Opcode::Literal) {
        return nullptr;
    }

    bool result = true;

    if (binary_expr.left->type_desc == NUMBER) {
        auto ln = dynamic_cast<NumberLiteral*>(binary_expr.left.get())->value;
        auto rn = dynamic_cast<NumberLiteral*>(binary_expr.right.get())->value;
        switch (binary_expr.opcode) {
            case Opcode::Less:
                result = ln < rn;
                break;
            case Opcode::LessEqual:
                result = ln <= rn;
                break;
            case Opcode::Greater:
                result = ln > rn;
                break;
            case Opcode::GreaterEqual:
                result = ln >= rn;
                break;
            case Opcode::Equal:
                result = ln == rn;
                break;
            case Opcode::NotEqual:
                result = ln != rn;
                break;
            default:
                break;
        }
        return std::make_unique<BooleanLiteral>(binary_expr.loc, result);
    } else if (binary_expr.left->type_desc == STRING) {
        auto ln = dynamic_cast<StringLiteral*>(binary_expr.left.get())->value;
        auto rn = dynamic_cast<StringLiteral*>(binary_expr.right.get())->value;
        switch (binary_expr.opcode) {
            case Opcode::Less:
                result = ln < rn;
                break;
            case Opcode::LessEqual:
                result = ln <= rn;
                break;
            case Opcode::Greater:
                result = ln > rn;
                break;
            case Opcode::GreaterEqual:
                result = ln >= rn;
                break;
            case Opcode::Equal:
                result = ln == rn;
                break;
            case Opcode::NotEqual:
                result = ln != rn;
                break;
            default:
                break;
        }
        return std::make_unique<BooleanLiteral>(binary_expr.loc, result);
    } else if (binary_expr.left->type_desc == BOOLEAN) {
        auto ln = dynamic_cast<BooleanLiteral*>(binary_expr.left.get())->value;
        auto rn = dynamic_cast<BooleanLiteral*>(binary_expr.right.get())->value;
        switch (binary_expr.opcode) {
            case Opcode::Equal:
                result = ln == rn;
                break;
            case Opcode::NotEqual:
                result = ln != rn;
                break;
            default:
                break;
        }
    } else {
        return nullptr;
    }

    return std::make_unique<BooleanLiteral>(binary_expr.loc, result);
}

auto reduce_logical(BinaryExpression& binary_expr)
    -> std::unique_ptr<Expression> {
    auto const& lb = dynamic_cast<BooleanLiteral*>(binary_expr.left.get());
    auto const& rb = dynamic_cast<BooleanLiteral*>(binary_expr.right.get());

    switch (binary_expr.opcode) {
        case Opcode::And:
            if (lb) {
                if (lb->value) {
                    return std::move(binary_expr.right);
                } else {
                    return std::make_unique<BooleanLiteral>(lb->loc, false);
                }
            } else if (rb) {
                if (rb->value) {
                    return std::move(binary_expr.left);
                } else {
                    return std::make_unique<BooleanLiteral>(rb->loc, false);
                }
            }
            break;
        case Opcode::Or:
            if (lb) {
                if (lb->value) {
                    return std::make_unique<BooleanLiteral>(lb->loc, true);
                } else {
                    return std::move(binary_expr.right);
                }
            } else if (rb) {
                if (rb->value) {
                    return std::make_unique<BooleanLiteral>(rb->loc, true);
                } else {
                    return std::move(binary_expr.left);
                }
            }
        default:
            break;
    }

    return nullptr;
}

auto convert_to_str(std::unique_ptr<Expression> number)
    -> std::unique_ptr<Expression> {
    auto lrt = std::make_unique<UnaryExpression>(
        number->loc,
        std::move(number),
        Opcode::CallLRT
    );

    lrt->idNdx = 5;
    lrt->type_desc = STRING;

    return lrt;
}

auto reduce_addition(BinaryExpression& binary_expr)
    -> std::unique_ptr<Expression> {
    auto const& ls = dynamic_cast<StringLiteral*>(binary_expr.left.get());
    auto const& rs = dynamic_cast<StringLiteral*>(binary_expr.right.get());
    auto const& ln = dynamic_cast<NumberLiteral*>(binary_expr.left.get());
    auto const& rn = dynamic_cast<NumberLiteral*>(binary_expr.right.get());

    if (ls) {
        if (rs) {
            return std::make_unique<StringLiteral>(
                binary_expr.loc,
                ls->value + rs->value,
                ls->raw_value + rs->raw_value
            );
        } else if (ls->value.empty()) {
            if (binary_expr.right->type_desc == STRING) {
                return std::move(binary_expr.right);
            } else {
                return convert_to_str(std::move(binary_expr.right));
            }
        } else if (rn) {
            auto as_str = std::to_string(rn->value);
            return std::make_unique<StringLiteral>(
                binary_expr.loc,
                ls->value + as_str,
                ls->raw_value + as_str
            );
        }
    }

    else if (rs) {
        if (rs->value.empty()) {
            if (binary_expr.left->type_desc == STRING) {
                return std::move(binary_expr.left);
            } else {
                return convert_to_str(std::move(binary_expr.left));
            }
        } else if (ln) {
            auto as_str = std::to_string(ln->value);
            return std::make_unique<StringLiteral>(
                binary_expr.loc,
                as_str + rs->value,
                as_str + rs->raw_value
            );
        }
    }

    else if (ln) {
        if (rn) {
            return std::make_unique<NumberLiteral>(
                ln->loc,
                ln->value + rn->value
            );
        } else if (ln->value == 0 && binary_expr.right->type_desc == NUMBER) {
            return std::move(binary_expr.right);
        }
    }

    else if (rn && rn->value == 0 && binary_expr.left->type_desc == NUMBER) {
        return std::move(binary_expr.left);
    }

    if (binary_expr.left->type_desc == NUMBER
        && binary_expr.right->type_desc == STRING) {
        binary_expr.left = convert_to_str(std::move(binary_expr.left));
    } else if (binary_expr.left->type_desc == STRING
               && binary_expr.right->type_desc == NUMBER) {
        binary_expr.right = convert_to_str(std::move(binary_expr.right));
    }

    return nullptr;
}

auto reduce_subtraction(BinaryExpression& binary_expr)
    -> std::unique_ptr<Expression> {
    auto const& ln = dynamic_cast<NumberLiteral*>(binary_expr.left.get());
    auto const& rn = dynamic_cast<NumberLiteral*>(binary_expr.right.get());

    if (ln) {
        if (rn) {
            return std::make_unique<NumberLiteral>(
                ln->loc,
                ln->value + rn->value
            );
        } else if (ln->value == 0) {
            return std::make_unique<UnaryExpression>(
                binary_expr.loc,
                std::move(binary_expr.left),
                Opcode::Minus
            );
        }
    }

    else if (rn && rn->value == 0) {
        return std::move(binary_expr.left);
    }

    return nullptr;
}

auto reduce_multiplication(BinaryExpression& binary_expr)
    -> std::unique_ptr<Expression> {
    auto const& ln = dynamic_cast<NumberLiteral*>(binary_expr.left.get());
    auto const& rn = dynamic_cast<NumberLiteral*>(binary_expr.right.get());

    if (ln) {
        if (rn) {
            return std::make_unique<NumberLiteral>(
                ln->loc,
                ln->value * rn->value
            );
        } else if (ln->value == 0) {
            return std::make_unique<NumberLiteral>(ln->loc, 0);
        }
    }

    else if (rn && rn->value == 0) {
        return std::make_unique<NumberLiteral>(rn->loc, 0);
    }

    return nullptr;
}

auto reduce_division(BinaryExpression& binary_expr)
    -> std::unique_ptr<Expression> {
    auto const& ln = dynamic_cast<NumberLiteral*>(binary_expr.left.get());
    auto const& rn = dynamic_cast<NumberLiteral*>(binary_expr.right.get());

    if (rn) {
        if (rn->value == 0) {
            // todo: error here
            return std::make_unique<NumberLiteral>(rn->loc, 0);
        } else if (rn->value == 1) {
            return std::move(binary_expr.left);
        } else if (ln) {
            return std::make_unique<NumberLiteral>(
                ln->loc,
                ln->value / rn->value
            );
        }
    }

    else if (ln && ln->value == 0) {
        return std::make_unique<NumberLiteral>(ln->loc, 0);
    }

    return nullptr;
}

} // namespace dgeval::ast
