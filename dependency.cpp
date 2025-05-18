#include "dependency.hpp"
#include <memory>
#include <print>
#include <queue>
#include <unordered_set>
#include <vector>
#include "ast.hpp"
#include "context.hpp"

namespace dgeval::ast {

void Dependency::visit_program(Program& program) {
    auto& statements = program.statements;

    statements->accept(*this);

    std::queue<size_t> queue;
    std::vector<int> in_degree(statements->inner.size());
    std::vector<std::unique_ptr<Statement>> sorted;
    std::vector<std::unique_ptr<Statement>> circular;

    // todo: remove unresolved variables

    for (auto const& [symbol, idxs] : dependents) {
        std::println("{} provides: {}", symbol, idxs);
        for (auto idx : idxs) {
            in_degree[idx] += 1;
        }
    }

    for (size_t idx = 0; idx < defined_symbols.size(); idx++) {
        std::println("{} defines: {}", idx, defined_symbols[idx]);
    }

    for (size_t idx = 0; idx < in_degree.size(); ++idx) {
        if (in_degree[idx] == 0) {
            queue.push(idx);
        }
    }

    for (; !queue.empty(); queue.pop()) {
        size_t idx = queue.front();
        sorted.push_back(std::move(statements->inner[idx]));

        for (auto const& symbol : defined_symbols[idx]) {
            for (auto idx : dependents[symbol]) {
                if (--in_degree[idx] == 0) {
                    queue.push(idx);
                }
            }
        }
    }

    for (size_t idx = 0; idx < in_degree.size(); ++idx) {
        if (in_degree[idx] != 0) {
            circular.push_back(std::move(statements->inner[idx]));
        }
    }

    program.circular_statements =
        std::make_unique<StatementList>(std::move(circular));
    program.statements = std::make_unique<StatementList>(std::move(sorted));
}

void Dependency::visit_statement_list(StatementList& statements) {
    for (size_t idx = 0; idx < statements.inner.size(); ++idx) {
        statement_idx = idx;
        defined_symbols.emplace_back();
        statements.inner[idx]->accept(*this);
    }
}

void Dependency::visit_expression_statement(ExpressionStatement& statement) {
    statement.expression->accept(*this);
}

void Dependency::visit_wait_statement(WaitStatement& statement) {
    for (auto const& id : statement.id_list) {
        dependents[id].insert(statement_idx);
    }

    statement.expression->accept(*this);
}

void Dependency::visit_expression(Expression& expression) {}

void Dependency::visit_number(NumberLiteral& number) {}

void Dependency::visit_string(StringLiteral& string) {}

void Dependency::visit_boolean(BooleanLiteral& boolean) {}

void Dependency::visit_array(ArrayLiteral& array) {
    opcode = Opcode::None;
    array.items->accept(*this);
}

void Dependency::visit_identifier(Identifier& identifier) {
    if (opcode == Opcode::Assign) {
        defined_symbols[statement_idx].insert(identifier.id);
    } else if (opcode != Opcode::Call || dependents.contains(identifier.id)) {
        dependents[identifier.id].insert(statement_idx);
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
