#pragma once

#include <algorithm>
#include <optional>
#include <unordered_map>
#include "ast.hpp"
#include "intermediate_code.hpp"

namespace dgeval::ast {

enum class MessageSeverity : std::uint8_t {
    Info,
    Warning,
    Error,
};

const std::array<std::string, 3> SEVERITY_STR = {"Info", "Warning", "Error"};

struct Message {
    Message(std::string text) :
        text(std::move(text)),
        severity(MessageSeverity::Info) {}

    Message(location const& loc, std::string text) :
        loc(loc),
        text(std::move(text)) {}

    Message(int line, std::string text) :
        loc(location(position(nullptr, line, 1))),
        text(std::move(text)) {}

    std::optional<location> loc;
    std::string text;
    MessageSeverity severity {MessageSeverity::Error};
};

class Program {
  public:
    Program(std::unique_ptr<StatementList> statements) :
        statements(std::move(statements)) {}

    void accept(Visitor<void>& visitor) {
        visitor.visit_program(*this);
    }

    auto accept(Visitor<std::unique_ptr<Expression>>& visitor)
        -> std::unique_ptr<Expression> {
        return visitor.visit_program(*this);
    }

    void sort_messages() {
        std::ranges::sort(messages, [](Message const& a, Message const& b) {
            if (a.loc.has_value() && b.loc.has_value()) {
                position const& loc_a = a.loc->begin;
                position const& loc_b = b.loc->begin;
                return loc_a.line < loc_b.line
                    || (loc_a.line == loc_b.line && loc_a.column < loc_b.column
                    );
            }

            return false;
        });
    }

    std::unique_ptr<StatementList> statements;
    std::unordered_map<std::string, SymbolDescriptor> symbol_table;
    std::unique_ptr<StatementList> circular_statements;
    std::vector<Instruction> instructions;
    std::vector<Message> messages;
};

} // namespace dgeval::ast
