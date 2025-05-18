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
    std::ofstream& output,
    std::vector<std::string>& strings,
    const std::string& delimiter
) {
    for (auto str = strings.begin(); str != strings.end();) {
        std::print(output, "\"{}\"", *str);
        if (++str != strings.end()) {
            std::print(output, ", ");
        }
    }
}

void Printer::visit_program(Program& program) {
    std::print(output, R"({{"circularStatements": )");
    program.circular_statements->accept(*this);

    auto& symbols = program.symbol_table;
    auto& messages = program.messages;
    std::print(output, R"(, "symbols": )");
    std::print(output, "[");
    for (auto symbol = symbols.begin(); symbol != symbols.end();) {
        int type = std::to_underlying(symbol->second.type);
        std::print(output, R"({{"name": "{}",)", symbol->first);
        std::print(output, R"("type": "{}",)", type_str[type]);
        std::print(output, R"("dim": {}}})", symbol->second.dimension);

        if (++symbol != symbols.end()) {
            std::print(output, ", ");
        }
    }
    std::print(output, "]");

    std::print(output, R"(, "executableStatements": )");
    program.statements->accept(*this);

    program.sort_messages();
    std::print(output, R"(, "messages": )");
    std::print(output, "[");
    for (auto msg = messages.begin(); msg != messages.end();) {
        std::print(output, "\"");
        if (msg->loc.has_value()) {
            std::print(
                output,
                "Line Number {}:{} ",
                msg->loc->begin.line,
                msg->loc->begin.column
            );
        }
        std::print(
            output,
            R"([{}]: {}.")",
            SEVERITY_STR[std::to_underlying(msg->severity)],
            msg->text
        );
        if (++msg != messages.end()) {
            std::print(output, ", ");
        }
    }
    std::print(output, "]");

    std::print(output, "}}");
}

void Printer::visit_statement_list(StatementList& statements) {
    std::print(output, "[");
    for (auto statement = statements.inner.begin();
         statement != statements.inner.end();) {
        (*statement)->accept(*this);
        if (++statement != statements.inner.end()) {
            std::print(output, ", ");
        }
    }
    std::print(output, "]");
}

void Printer::visit_expression_statement(ExpressionStatement& statement) {
    std::print(output, R"({{"lineNumber": {}, )", statement.line_number);
    std::print(output, R"("nodeType": "expression statement", )");
    std::print(output, R"("expression": )");
    statement.expression->accept(*this);
    std::print(output, "}}");
}

void Printer::visit_wait_statement(WaitStatement& statement) {
    std::print(output, R"({{"lineNumber": {}, )", statement.line_number);
    std::print(output, R"("nodeType": "wait statement", )");
    std::print(output, R"("expression": )");
    statement.expression->accept(*this);
    std::print(output, R"(, "idList": [)");
    join_strings(output, statement.id_list, ", ");
    std::print(output, "]}}");
}

void Printer::visit_expression(Expression& expression) {
    int opcode = std::to_underlying(expression.opcode);
    int type = std::to_underlying(expression.type_desc.type);

    std::print(output, R"({{"lineNumber": {},)", expression.loc.begin.line);
    std::print(output, R"("nodeType": "expression node",)");
    std::print(output, R"("opCode": {},)", opcode);
    std::print(output, R"("mnemonic": "{}", )", mnemonic[opcode]);
    std::print(output, R"("typeCode": {},)", type);
    std::print(output, R"("type": "{}",)", type_str[type]);
    std::print(output, R"("dim": {},)", expression.type_desc.dimension);
}

void Printer::visit_number(NumberLiteral& number) {
    visit_expression(number);
    std::print(output, R"("numberValue": "{}"}})", number.value);
}

void Printer::visit_string(StringLiteral& string) {
    visit_expression(string);
    std::print(output, R"("stringValue": "{}"}})", string.raw_value);
}

void Printer::visit_boolean(BooleanLiteral& boolean) {
    visit_expression(boolean);
    std::print(output, R"("booleanValue": "{}"}})", boolean.value);
}

void Printer::visit_array(ArrayLiteral& array) {
    visit_expression(array);
    std::print(output, R"("left": )");
    if (array.items) {
        array.items->accept(*this);
    } else {
        std::print(output, "{{}}");
    }
    std::print(output, "}}");
}

void Printer::visit_identifier(Identifier& identifier) {
    visit_expression(identifier);
    std::print(output, R"("id": "{}"}})", identifier.id);
}

void Printer::visit_binary_expression(BinaryExpression& binary_expr) {
    visit_expression(binary_expr);
    std::print(output, R"("left": )");
    binary_expr.left->accept(*this);
    std::print(output, R"(, "right": )");
    if (binary_expr.right) {
        binary_expr.right->accept(*this);
    } else {
        std::print(output, "{{}}");
    }
    std::print(output, "}}");
}

void Printer::visit_unary_expression(UnaryExpression& unary_expr) {
    visit_expression(unary_expr);
    std::print(output, R"("left": )");
    unary_expr.left->accept(*this);
    std::print(output, "}}");
}

} // namespace dgeval::ast
