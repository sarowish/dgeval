#pragma once

#include <cstdint>
#include <unordered_map>
#include "ast.hpp"

namespace dgeval::ast {

enum class MessageSeverity : std::uint8_t {
    Info,
    Warning,
    Error,
};

struct Message {
    Message(int line_number, std::string text) :
        line_number(line_number),
        text(std::move(text)) {}

    int line_number;
    std::string text;
    MessageSeverity severity {MessageSeverity::Error};
};

class Program {
  public:
    Program(std::unique_ptr<StatementList> statements) :
        statements(std::move(statements)) {}

    void accept(Visitor& visitor) {
        visitor.visit_program(*this);
    }

    std::unique_ptr<StatementList> statements;
    std::unordered_map<std::string, TypeDescriptor> symbol_table;
    std::unique_ptr<StatementList> circular_statements;
    std::vector<Message> messages;
};

} // namespace dgeval::ast
