#include "printer.hpp"
#include <print>
#include "ast.hpp"

namespace dgeval::ast {

void join_strings(std::ofstream& output, std::vector<std::string>& strings) {
    for (auto str = strings.begin(); str != strings.end();) {
        std::print(output, R"("{}")", *str);
        if (++str != strings.end()) {
            std::print(output, ", ");
        }
    }
}

auto escape_string(std::string const& str) -> std::string {
    static char const* hex_digits = "0123456789ebcdef";
    std::string escaped;

    for (auto ch : str) {
        switch (ch) {
            case '\n':
                escaped += "\\n";
                break;
            case '\t':
                escaped += "\\t";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\\':
                escaped += "\\\\";
                break;
            default:
                if (ch < 32) {
                    escaped += "\\x";
                    escaped += hex_digits[ch >> 4];
                    escaped += hex_digits[ch & 0xf];
                } else {
                    escaped += ch;
                }
        }
    }

    return escaped;
}

void print_ic(
    const std::string& file_name,
    const std::vector<Instruction>& instructions
) {
    std::ofstream output(file_name);

    for (int idx = 0; idx < instructions.size(); ++idx) {
        auto const& instruction = instructions[idx];

        std::print(
            output,
            "{:05} {:6}{:5} type:[{}:{}] ",
            idx,
            MNEMONICS[std::to_underlying(instruction.opcode)],
            instruction.parameter,
            TYPE_STR[std::to_underlying(instruction.type.type)],
            instruction.type.dimension
        );

        auto const& value = instruction.value;

        if (std::holds_alternative<double>(value)) {
            std::print(output, "{}", get<double>(value));
        } else if (std::holds_alternative<std::string>(value)) {
            auto const& str = get<std::string>(value);
            std::print(output, R"("{}")", escape_string(str));
            if (instruction.opcode == Opcode::Call) {
                std::print(output, " @{}", RUNTIME_LIBRARY.at(str).idNdx);
            }
        } else if (std::holds_alternative<bool>(value)) {
            std::print(output, "{}", get<bool>(value));
        }

        std::println(output);
    }
}

void Printer::visit_program(Program& program) {
    std::print(output, R"({{"circularStatements": )");
    program.circular_statements->accept(*this);

    auto& symbols = program.symbol_table;
    auto& instructions = program.instructions;
    auto& messages = program.messages;

    std::vector<std::pair<std::string, SymbolDescriptor>> sorted_symbols;

    sorted_symbols.reserve(symbols.size());
    for (auto& entry : symbols) {
        sorted_symbols.emplace_back(entry);
    }

    std::sort(
        sorted_symbols.begin(),
        sorted_symbols.end(),
        [](std::pair<std::string, SymbolDescriptor> const& a,
           std::pair<std::string, SymbolDescriptor> const& b) {
            return a.second.idx < b.second.idx;
            ;
        }
    );

    std::print(output, R"(, "symbols": [)");
    for (auto symbol = sorted_symbols.begin();
         symbol != sorted_symbols.end();) {
        const int type = std::to_underlying(symbol->second.type_desc.type);
        std::print(
            output,
            R"({{"name": "{}", "type": "{}", "dim": {}}})",
            symbol->first,
            TYPE_STR[type],
            symbol->second.type_desc.dimension
        );

        if (++symbol != sorted_symbols.end()) {
            std::print(output, ", ");
        }
    }

    std::print(output, R"(], "executablestatements": )");
    program.statements->accept(*this);

    std::print(output, R"(, "ic": [)");
    for (auto instruction = instructions.begin();
         instruction != instructions.end();) {
        int const opcode = std::to_underlying(instruction->opcode);
        int const type = std::to_underlying(instruction->type.type);

        std::print(
            output,
            R"({{"mnemonic": "{}", "opCode": {}, "type": {}, "p1": {}, "dim": {})",
            MNEMONICS[opcode],
            opcode,
            type,
            instruction->parameter,
            instruction->type.dimension
        );

        auto const& value = instruction->value;

        switch (instruction->opcode) {
            case Opcode::Call: {
                auto const& function_name = get<std::string>(value);
                std::print(
                    output,
                    R"(, "name": "{}", "id": {})",
                    function_name,
                    RUNTIME_LIBRARY.at(function_name).idNdx
                );
            } break;
            case Opcode::Identifier:
                std::print(output, R"(, "id": "{}")", get<std::string>(value));
                break;
            case Opcode::Assign:
                break;
            default:
                if (std::holds_alternative<double>(value)) {
                    std::print(output, R"(, "value": {})", get<double>(value));
                } else if (std::holds_alternative<std::string>(value)) {
                    std::print(
                        output,
                        R"(, "value": "{}")",
                        escape_string(get<std::string>(value))
                    );
                } else if (std::holds_alternative<bool>(value)) {
                    std::print(output, R"(, "value": {})", get<bool>(value));
                }
                break;
        }

        std::print(output, "}}");

        if (++instruction != instructions.end()) {
            std::print(output, ", ");
        }
    }

    program.sort_messages();
    std::print(output, R"(], "messages": [)");
    for (auto msg = messages.begin(); msg != messages.end();) {
        std::print(output, "\"");
        if (msg->loc.has_value()) {
            std::print(output, "Line Number {} ", msg->loc->begin.line);
            std::print("Line Number {} ", msg->loc->begin.line);
        }

        std::print(
            output,
            R"([{}]: {}.")",
            SEVERITY_STR[std::to_underlying(msg->severity)],
            msg->text
        );

        std::println(
            "[{}]: {}.",
            SEVERITY_STR[std::to_underlying(msg->severity)],
            msg->text
        );

        if (++msg != messages.end()) {
            std::print(output, ", ");
        }
    }
    std::print(output, "]}}");
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
    std::print(
        output,
        R"({{"lineNumber": {}, "nodeType": "expression statement", "expression": )",
        statement.line_number
    );
    statement.expression->accept(*this);
    std::print(output, "}}");
}

void Printer::visit_wait_statement(WaitStatement& statement) {
    std::print(
        output,
        R"({{"lineNumber": {}, "nodeType": "wait statement", "expression": )",
        statement.line_number
    );
    statement.expression->accept(*this);
    std::print(output, R"(, "idList": [)");
    join_strings(output, statement.id_list);
    std::print(output, "]}}");
}

void Printer::visit_expression(Expression& expression) {
    int const opcode = std::to_underlying(expression.opcode);
    int const type = std::to_underlying(expression.type_desc.type);

    std::print(
        output,
        R"({{"lineNumber": {}, "nodeType": "expression node", "opCode": {}, "mnemonic": "{}", "typeCode": {}, "type": "{}", "dim": {}, "idNdx": {})",
        expression.loc.begin.line,
        opcode,
        MNEMONICS[opcode],
        type,
        TYPE_STR[type],
        expression.type_desc.dimension,
        expression.idNdx
    );
}

void Printer::visit_number(NumberLiteral& number) {
    visit_expression(number);
    std::print(output, R"(, "numberValue": "{}"}})", number.value);
}

void Printer::visit_string(StringLiteral& string) {
    visit_expression(string);
    std::print(output, "}}");
}

void Printer::visit_boolean(BooleanLiteral& boolean) {
    visit_expression(boolean);
    std::print(output, R"(, "numberValue": "{}"}})", boolean.value);
}

void Printer::visit_array(ArrayLiteral& array) {
    visit_expression(array);
    std::print(output, R"(, "left": )");
    array.items->accept(*this);
    std::print(output, "}}");
}

void Printer::visit_identifier(Identifier& identifier) {
    visit_expression(identifier);
    std::print(output, R"(, "id": "{}"}})", identifier.id);
}

void Printer::visit_binary_expression(BinaryExpression& binary_expr) {
    visit_expression(binary_expr);
    std::print(output, R"(, "left": )");
    binary_expr.left->accept(*this);
    if (binary_expr.right) {
        std::print(output, R"(, "right": )");
        binary_expr.right->accept(*this);
    }
    std::print(output, "}}");
}

void Printer::visit_unary_expression(UnaryExpression& unary_expr) {
    visit_expression(unary_expr);
    std::print(output, R"(, "left": )");
    unary_expr.left->accept(*this);
    std::print(output, "}}");
}

} // namespace dgeval::ast
