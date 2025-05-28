#include "dependency.hpp"
#include <queue>

namespace dgeval::ast {

void Dependency::visit_program(Program& program) {
    auto& statements = program.statements;

    statements->accept(*this);

    std::queue<size_t> queue;
    std::vector<int> in_degree(statements->inner.size());
    std::vector<std::unordered_set<size_t>> relations(statements->inner.size());
    std::vector<std::unique_ptr<Statement>> sorted;
    std::vector<std::unique_ptr<Statement>> circular;

    for (auto const& [symbol, rel] : symbols) {
        for (auto parent : rel.defines) {
            for (auto child : rel.depends) {
                relations[parent].insert(child);
                ++in_degree[child];
            }
        }
    }

    for (size_t idx = 0; idx < in_degree.size(); ++idx) {
        if (in_degree[idx] == 0) {
            queue.push(idx);
        }
    }

    for (; !queue.empty(); queue.pop()) {
        const size_t idx = queue.front();
        sorted.push_back(std::move(statements->inner[idx]));

        for (auto const& idx : relations[idx]) {
            if (--in_degree[idx] == 0) {
                queue.push(idx);
            }
        }
    }

    for (auto const& [symbol, rel] : symbols) {
        for (auto defining : rel.defines) {
            if (!statements->inner[defining]) {
                program.symbol_table[symbol].type_desc = NONE;
            }
        }
    }

    for (size_t idx = 0; idx < in_degree.size(); ++idx) {
        if (in_degree[idx] != 0) {
            circular.push_back(std::move(statements->inner[idx]));
        }
    }

    int idNdx = 0;

    for (auto& [_, descriptor] : program.symbol_table) {
        descriptor.idx = idNdx++;
    }

    program.circular_statements =
        std::make_unique<StatementList>(std::move(circular));
    program.statements = std::make_unique<StatementList>(std::move(sorted));
}

void Dependency::visit_statement_list(StatementList& statements) {
    for (size_t idx = 0; idx < statements.inner.size(); ++idx) {
        statement_idx = idx;
        statements.inner[idx]->accept(*this);
    }
}

void Dependency::visit_expression_statement(ExpressionStatement& statement) {
    statement.expression->accept(*this);
}

void Dependency::visit_wait_statement(WaitStatement& statement) {
    for (auto const& id : statement.id_list) {
        symbols[id].depends.insert(statement_idx);
    }

    statement.expression->accept(*this);
}

void Dependency::visit_expression(Expression& expression) {}

void Dependency::visit_number(NumberLiteral& number) {}

void Dependency::visit_string(StringLiteral& string) {}

void Dependency::visit_boolean(BooleanLiteral& boolean) {}

void Dependency::visit_array(ArrayLiteral& array) {
    opcode = Opcode::None;
    if (array.items) {
        array.items->accept(*this);
    }
}

void Dependency::visit_identifier(Identifier& identifier) {
    if (RUNTIME_LIBRARY.contains(identifier.id)) {
        return;
    }

    if (opcode == Opcode::Assign) {
        symbols[identifier.id].defines.insert(statement_idx);
    } else if (opcode != Opcode::Call) {
        symbols[identifier.id].depends.insert(statement_idx);
    }
}

void Dependency::visit_binary_expression(BinaryExpression& binary_expr) {
    opcode = binary_expr.opcode;
    binary_expr.left->accept(*this);
    opcode = Opcode::None;
    if (binary_expr.right) {
        binary_expr.right->accept(*this);
    }
}

void Dependency::visit_unary_expression(UnaryExpression& unary_expr) {
    opcode = Opcode::None;
    unary_expr.left->accept(*this);
}

} // namespace dgeval::ast
