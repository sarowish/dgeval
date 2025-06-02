#pragma once

#include <bitset>
#include <cstdint>
#include <utility>

namespace dgeval::ast {

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

} // namespace dgeval::ast
