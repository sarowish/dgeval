#pragma once

#include <bitset>
#include <cstdint>
#include <utility>
#include "linear_ir.hpp"

namespace dgeval::ast {

class Instruction;

enum class Optimization : std::uint8_t {
    DeadStatement,
    DeadExpressionPart,
    PeepholeOffload,
    PeepholeConstsink,
};

class OptimizationFlags {
    std::bitset<4> flags;

  public:
    OptimizationFlags() : flags(0b1111) {}

    OptimizationFlags(int flags) : flags(flags) {}

    void set(Optimization flag, bool value = true) {
        flags.set(std::to_underlying(flag), value);
    }

    constexpr auto operator[](Optimization flag) const -> bool {
        return flags[std::to_underlying(flag)];
    }
};

class Window {
  public:
    Window(Instruction* start, Instruction* end);
    auto ineffective_store_load() -> bool;
    auto constant_value_sink() -> bool;
    auto shift_at(size_t idx, int value) -> bool;
    auto shift(int value) -> bool;
    [[nodiscard]] auto last_idx() const -> size_t;
    [[nodiscard]] auto branches_end_with_literals() const -> bool;
    auto remove_literals(int offset) -> int;

    std::array<Instruction*, 3> inner;
    Instruction* end;
    Instruction* root;
    std::unique_ptr<Window> true_branch;
    std::unique_ptr<Window> false_branch;
    int offset {0};
};

class Peephole {
  public:
    Peephole(std::vector<Instruction>& instructions, OptimizationFlags flags);
    void run();
    void run_helper(Window& window);
    void apply_removal();

    std::vector<Instruction>& instructions;
    bool optimize_offload {true};
    bool optimize_const_sink {true};
};

auto follow_jumps(std::vector<Instruction>& instructions, Instruction* start)
    -> Instruction*;

} // namespace dgeval::ast
