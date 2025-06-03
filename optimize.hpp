#pragma once

#include <bitset>
#include <cstdint>
#include <utility>
#include "intermediate_code.hpp"

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
    int offset {0};

  public:
    auto ineffective_store_load() -> bool;
    auto constant_value_sink() -> bool;
    auto shift(int value) -> bool;
    void update_jump_location();

    std::array<Instruction*, 3> inner;
    Instruction* end;
};

class Peephole {
  public:
    Peephole(std::vector<Instruction>& instructions, OptimizationFlags flags);
    void uuh();
    void remove_nop();

    std::vector<Instruction>& instructions;
    bool optimize_offload {true};
    bool optimize_const_sink {true};
};

} // namespace dgeval::ast
