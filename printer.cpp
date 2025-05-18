#include "printer.hpp"
#include <cassert>
#include <memory>
#include <print>
#include <string>
#include <unordered_map>
#include <utility>
#include "ast.hpp"
#include "context.hpp"

namespace dgeval::ast {

void join_strings(
    std::vector<std::string>& strings,
    const std::string& delimiter
) {
    for (auto str = strings.begin(); str != strings.end();) {
        std::print("\"{}\"", *str);
        if (++str != strings.end()) {
            std::print(", ");
        }
    }
}

void Printer::visit_program(Program& program) {
    std::print(R"({{"circularStatements": )");
    program.circular_statements->accept(*this);

    auto& symbols = program.symbol_table;
    auto& messages = program.messages;
    std::print(R"(, "symbols": )");
    std::print("[");
    for (auto symbol = symbols.begin(); symbol != symbols.end();) {
        int type = std::to_underlying(symbol->second.type);
        std::print(R"({{"name": "{}",)", symbol->first);
        std::print(R"("type": "{}",)", type_str[type]);
        std::print(R"("dim": "{}"}})", symbol->second.dimension);

        if (++symbol != symbols.end()) {
            std::print(", ");
        }
    }
    std::print("]");

    std::print(R"(, "executableStatements": )");
    program.statements->accept(*this);

    std::print(R"(, "messages": )");
    std::print("[");
    for (auto msg = messages.begin(); msg != messages.end();) {
        std::print(
            R"("Line number {} [{}]: {}.")",
            msg->line_number,
            "Error",
            msg->text
        );
        if (++msg != messages.end()) {
            std::print(", ");
        }
    }
    std::print("]");

    std::print("}}");
}

void Printer::visit_statement_list(StatementList& statements) {
    std::print("[");
    for (auto statement = statements.inner.begin();
         statement != statements.inner.end();) {
        (*statement)->accept(*this);
        if (++statement != statements.inner.end()) {
            std::print(", ");
        }
    }
    std::print("]");
}

void Printer::visit_expression_statement(ExpressionStatement& statement) {
    std::print(R"({{"lineNumber": {}, )", statement.line_number);
    std::print(R"("nodeType": "expression statement", )");
    std::print(R"("expression": )");
    statement.expression->accept(*this);
    std::print("}}");
}

void Printer::visit_wait_statement(WaitStatement& statement) {
    std::print(R"({{"lineNumber": {}, )", statement.line_number);
    std::print(R"("nodeType": "wait statement", )");
    std::print(R"("expression": )");
    statement.expression->accept(*this);
    std::print(R"(, "idList": [)");
    join_strings(statement.id_list, ", ");
    std::print("]}}");
}

void Printer::visit_expression(Expression& expression) {
    int opcode = std::to_underlying(expression.opcode);
    int type = std::to_underlying(expression.type_desc.type);

    std::print(R"({{"lineNumber": {},)", expression.line_number);
    std::print(R"("nodeType": "expression node",)");
    std::print(R"("opCode": {},)", opcode);
    std::print(R"("mnemonic": "{}", )", mnemonic[opcode]);
    std::print(R"("typeCode": {},)", type);
    std::print(R"("type": "{}",)", type_str[type]);
    std::print(R"("dim": {},)", expression.type_desc.dimension);
}

void Printer::visit_number(NumberLiteral& number) {
    visit_expression(number);
    std::print(R"("numberValue": "{}"}})", number.value);
}

void Printer::visit_string(StringLiteral& string) {
    visit_expression(string);
    std::print(R"("stringValue": "{}"}})", string.value);
}

void Printer::visit_boolean(BooleanLiteral& boolean) {
    visit_expression(boolean);
    std::print(R"("booleanValue": "{}"}})", boolean.value);
}

void Printer::visit_array(ArrayLiteral& array) {
    visit_expression(array);
    std::print(R"("left": )");
    if (array.items) {
        array.items->accept(*this);
    }
    std::print("}}");
}

void Printer::visit_identifier(Identifier& identifier) {
    visit_expression(identifier);
    std::print(R"("id": "{}"}})", identifier.id);
}

void Printer::visit_binary_expression(BinaryExpression& binary_expr) {
    visit_expression(binary_expr);
    std::print(R"("left": )");
    binary_expr.left->accept(*this);
    std::print(R"(, "right": )");
    if (binary_expr.right) {
        binary_expr.right->accept(*this);
    } else {
        std::print("{{}}");
    }
    std::print("}}");
}

void Printer::visit_unary_expression(UnaryExpression& unary_expr) {
    visit_expression(unary_expr);
    std::print(R"("left": )");
    unary_expr.left->accept(*this);
    std::print("}}");
}

} // namespace dgeval::ast
