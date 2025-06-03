#include "optimize.hpp"

namespace dgeval::ast {

auto Window::ineffective_store_load() -> bool {
    // std::cout << "0: " << MNEMONICS[std::to_underlying(inner[0]->opcode)]
    //           << std::endl;
    // std::cout << "1: " << MNEMONICS[std::to_underlying(inner[1]->opcode)]
    //           << std::endl;
    // std::cout << "2: " << MNEMONICS[std::to_underlying(inner[2]->opcode)]
    //           << std::endl;
    // std::cout << "--------------" << std::endl;

    if (inner[1]->opcode != Opcode::Pop) {
        return false;
    }

    auto const first = inner[0];
    auto const third = inner[2];

    if (first->opcode == Opcode::Assign && third->opcode == Opcode::Identifier
        && first->parameter == third->parameter) {
        // std::cout << "optimized" << std::endl;
        offset += 2;
        inner[1]->opcode = Opcode::None;
        inner[1] += 2;
        inner[2]->opcode = Opcode::None;
        inner[2] += 2;
        return true;
    }

    return false;
}

auto Window::constant_value_sink() -> bool {
    if (inner[0]->is_literal() && inner[1]->opcode == Opcode::Pop) {
        offset += 2;
        inner[0]->opcode = Opcode::None;
        inner[1]->opcode = Opcode::None;
        shift(2);
        return true;
    }

    return false;
}

auto Window::shift(int value) -> bool {
    do {
        inner[0] += 1;
    } while (inner[0]->opcode == Opcode::None);

    inner[1] += 1;
    inner[2] += 1;

    return inner[1] == end;
}

void Window::update_jump_location() {
    if (inner[0]->opcode == Opcode::Jump
        || inner[0]->opcode == Opcode::JumpFalse) {
        inner[0]->parameter -= offset;
    }
}

Peephole::Peephole(
    std::vector<Instruction>& instructions,
    OptimizationFlags flags
) :
    instructions(instructions),
    optimize_offload(flags[Optimization::PeepholeOffload]),
    optimize_const_sink(flags[Optimization::PeepholeConstsink]) {}

void Peephole::uuh() {
    if (!(optimize_offload || optimize_const_sink) || instructions.size() < 2) {
        return;
    }

    Window window;
    window.inner[0] = &instructions[0];
    window.inner[1] = &instructions[1];
    window.inner[2] = &instructions[2];
    window.end = instructions.end().base();

    while (true) {
        window.update_jump_location();

        if (optimize_offload && window.ineffective_store_load()) {
            continue;
        }

        if (optimize_const_sink) {
            window.constant_value_sink();
        }

        if (window.shift(1)) {
            window.constant_value_sink();
            break;
        }
    }

    auto removal = std::remove_if(
        instructions.begin(),
        instructions.end(),
        [](Instruction inst) { return inst.opcode == Opcode::None; }
    );

    instructions.erase(removal, instructions.end());
}

} // namespace dgeval::ast
