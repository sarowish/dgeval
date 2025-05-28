#include "printer.hpp"

namespace dgeval::ast {

void join_strings(std::ofstream& output, std::vector<std::string>& strings) {
    for (auto str = strings.begin(); str != strings.end();) {
        output << "\"" << *str << "\"";
        if (++str != strings.end()) {
            output << ", ";
        }
    }
}

void Printer::visit_program(Program& program) {
    output << "{\"circularStatements\": ";
    program.circular_statements->accept(*this);

    auto& symbols = program.symbol_table;
    auto& messages = program.messages;
    output << ", \"symbols\": [";
    for (auto symbol = symbols.begin(); symbol != symbols.end();) {
        const int type = std::to_underlying(symbol->second.type_desc.type);
        output << "{\"name\": \"" << symbol->first << "\"," << "\"type\": \""
               << TYPE_STR[type] << "\","
               << "\"dim\": " << symbol->second.type_desc.dimension << "}";

        if (++symbol != symbols.end()) {
            output << ", ";
        }
    }

    output << "], \"executableStatements\": ";
    program.statements->accept(*this);

    program.sort_messages();
    output << ", \"messages\": [";
    for (auto msg = messages.begin(); msg != messages.end();) {
        output << "\"";
        if (msg->loc.has_value()) {
            output << "Line Number " << msg->loc->begin.line << " ";
        }
        output << "[" << SEVERITY_STR[std::to_underlying(msg->severity)]
               << "]: " << msg->text << ".\"";
        if (++msg != messages.end()) {
            output << ", ";
        }
    }
    output << "]}";
}

void Printer::visit_statement_list(StatementList& statements) {
    output << "[";
    for (auto statement = statements.inner.begin();
         statement != statements.inner.end();) {
        (*statement)->accept(*this);
        if (++statement != statements.inner.end()) {
            output << ", ";
        }
    }
    output << "]";
}

void Printer::visit_expression_statement(ExpressionStatement& statement) {
    output << "{\"lineNumber\": " << statement.line_number << ", "
           << "\"nodeType\": \"expression statement\", "
           << "\"expression\": ";
    statement.expression->accept(*this);
    output << "}";
}

void Printer::visit_wait_statement(WaitStatement& statement) {
    output << "{\"lineNumber\": " << statement.line_number << ", "
           << "\"nodeType\": \"wait statement\", "
           << "\"expression\": ";
    statement.expression->accept(*this);
    output << ", \"idList\": [";
    join_strings(output, statement.id_list);
    output << "]}";
}

void Printer::visit_expression(Expression& expression) {
    int const opcode = std::to_underlying(expression.opcode);
    int const type = std::to_underlying(expression.type_desc.type);

    output << "{\"lineNumber\": " << expression.loc.begin.line
           << ", \"nodeType\": \"expression node\""
           << ", \"opCode\": " << opcode << ", \"mnemonic\": \""
           << MNEMONICS[opcode] << "\", \"typeCode\": " << type
           << ", \"type\": \"" << TYPE_STR[type]
           << "\", \"dim\": " << expression.type_desc.dimension
           << ", \"idNdx\": " << expression.idNdx;
}

void Printer::visit_number(NumberLiteral& number) {
    visit_expression(number);
    output << ", \"numberValue\": \"" << number.value << "\"}";
}

void Printer::visit_string(StringLiteral& string) {
    visit_expression(string);
    output << ", \"stringValue\": \"" << string.raw_value << "\"}";
}

void Printer::visit_boolean(BooleanLiteral& boolean) {
    visit_expression(boolean);
    output << ", \"numberValue\": \"" << boolean.value << "\"}";
}

void Printer::visit_array(ArrayLiteral& array) {
    visit_expression(array);
    if (array.items) {
        output << ", \"left\": ";
        array.items->accept(*this);
    }
    output << "}";
}

void Printer::visit_identifier(Identifier& identifier) {
    visit_expression(identifier);
    output << ", \"id\": \"" << identifier.id << "\"}";
}

void Printer::visit_binary_expression(BinaryExpression& binary_expr) {
    visit_expression(binary_expr);
    output << ", \"left\": ";
    binary_expr.left->accept(*this);
    if (binary_expr.right) {
        output << ", \"right\": ";
        binary_expr.right->accept(*this);
    }
    output << "}";
}

void Printer::visit_unary_expression(UnaryExpression& unary_expr) {
    visit_expression(unary_expr);
    output << ", \"left\": ";
    unary_expr.left->accept(*this);
    output << "}";
}

} // namespace dgeval::ast
