#include "printer.hpp"
#include <iomanip>
#include "ast.hpp"

namespace dgeval::ast {

void join_strings(std::ofstream& output, std::vector<std::string>& strings) {
    for (auto str = strings.begin(); str != strings.end();) {
        output << "\"" << *str << "\"";
        if (++str != strings.end()) {
            output << ", ";
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
        output << std::setfill('0') << std::setw(5) << idx << ' '
               << std::setfill(' ') << std::setw(6) << std::left
               << MNEMONICS[std::to_underlying(instruction.opcode)]
               << std::setw(5) << std::right << instruction.parameter
               << " type:["
               << TYPE_STR[std::to_underlying(instruction.type.type)] << ':'
               << instruction.type.dimension << "] ";

        auto const& value = instruction.value;

        if (std::holds_alternative<double>(value)) {
            output << get<double>(value);
        } else if (std::holds_alternative<std::string>(value)) {
            auto& str = get<std::string>(value);
            output << '"' << escape_string(str) << '"';
            if (instruction.opcode == Opcode::Call) {
                output << " @" << RUNTIME_LIBRARY.at(str).idNdx;
            }
        } else if (std::holds_alternative<bool>(value)) {
            output << get<bool>(value);
        }

        output << std::endl;
    }
}

void Printer::visit_program(Program& program) {
    output << "{\"circularStatements\": ";
    program.circular_statements->accept(*this);

    auto& symbols = program.symbol_table;
    auto& instructions = program.instructions;
    auto& messages = program.messages;

    std::vector<std::pair<std::string, SymbolDescriptor>> sorted_symbols;

    for (auto& entry : symbols) {
        sorted_symbols.push_back(entry);
    }

    std::sort(
        sorted_symbols.begin(),
        sorted_symbols.end(),
        [&](std::pair<std::string, SymbolDescriptor> a,
            std::pair<std::string, SymbolDescriptor> b) {
            return a.second.idx < b.second.idx;
            ;
        }
    );

    output << ", \"symbols\": [";
    for (auto symbol = sorted_symbols.begin();
         symbol != sorted_symbols.end();) {
        const int type = std::to_underlying(symbol->second.type_desc.type);
        output << "{\"name\": \"" << symbol->first << "\"," << "\"type\": \""
               << TYPE_STR[type] << "\","
               << "\"dim\": " << symbol->second.type_desc.dimension << "}";

        if (++symbol != sorted_symbols.end()) {
            output << ", ";
        }
    }

    output << "], \"executablestatements\": ";
    program.statements->accept(*this);

    output << ", \"ic\": [";
    for (auto instruction = instructions.begin();
         instruction != instructions.end();) {
        int const opcode = std::to_underlying(instruction->opcode);
        int const type = std::to_underlying(instruction->type.type);

        output << "{\"mnemonic\": \"" << MNEMONICS[opcode]
               << "\", \"opCode\": " << opcode << ", \"type\": " << type
               << ", \"p1\": " << instruction->parameter
               << ", \"dim\": " << instruction->type.dimension;

        auto const& value = instruction->value;

        switch (instruction->opcode) {
            case Opcode::Call: {
                auto& function_name = get<std::string>(value);
                output << ", \"name\": \"" << function_name << "\"";
                output << ", \"id\": "
                       << RUNTIME_LIBRARY.at(function_name).idNdx;
            } break;
            case Opcode::Identifier:
                output << ", \"id\": \"" << get<std::string>(value) << "\"";
                break;
            case Opcode::Assign:
                break;
            default:
                if (std::holds_alternative<double>(value)) {
                    output << ", \"value\": " << get<double>(value);
                } else if (std::holds_alternative<std::string>(value)) {
                    output << ", \"value\": \""
                           << escape_string(get<std::string>(value)) << "\"";
                } else if (std::holds_alternative<bool>(value)) {
                    output << ", \"value\": " << get<bool>(value);
                }
                break;
        }

        output << "}";

        if (++instruction != instructions.end()) {
            output << ", ";
        }
    }

    program.sort_messages();
    output << "], \"messages\": [";
    for (auto msg = messages.begin(); msg != messages.end();) {
        output << "\"";
        if (msg->loc.has_value()) {
            output << "Line Number " << msg->loc->begin.line << " ";
            std::cout << "Line Number " << msg->loc->begin.line << " ";
        }

        output << "[" << SEVERITY_STR[std::to_underlying(msg->severity)]
               << "]: " << msg->text << ".\"";

        std::cout << "[" << SEVERITY_STR[std::to_underlying(msg->severity)]
                  << "]: " << msg->text << "." << std::endl;

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
    output << "}";
}

void Printer::visit_boolean(BooleanLiteral& boolean) {
    visit_expression(boolean);
    output << ", \"numberValue\": \"" << boolean.value << "\"}";
}

void Printer::visit_array(ArrayLiteral& array) {
    visit_expression(array);
    output << ", \"left\": ";
    array.items->accept(*this);
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
