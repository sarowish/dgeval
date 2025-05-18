#pragma once

#include <algorithm>
#include <array>
#include <unordered_map>
#include "ast.hpp"
#include "location.hpp"

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

    void accept(Visitor& visitor) {
        visitor.visit_program(*this);
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
    std::unordered_map<std::string, TypeDescriptor> symbol_table;
    std::unique_ptr<StatementList> circular_statements;
    std::vector<Message> messages;
};

} // namespace dgeval::ast
