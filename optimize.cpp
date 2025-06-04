#include "optimize.hpp"

namespace dgeval::ast {

Window::Window(Instruction* start, Instruction* end) :
    inner({&start[0], &start[1], &start[2]}),
    end(end) {}

auto Window::ineffective_store_load() -> bool {
    if (inner[1]->opcode != Opcode::Pop) {
        return false;
    }

    auto const first = inner[0];
    auto const third = inner[2];

    if (first->opcode == Opcode::Assign && third->opcode == Opcode::Identifier
        && first->parameter == third->parameter) {
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

auto Window::shift_at(size_t idx, int value) -> bool {
    auto& inst = inner[idx];

    if (inst == end) {
        return true;
    }

    inst += value;

    while (inst != end && inst->opcode == Opcode::None) {
        inst += 1;
    }

    return inst == end;
}

auto Window::shift(int value) -> bool {
    shift_at(0, value);
    shift_at(1, value);
    shift_at(2, value);

    return inner[0] == end || inner[1] == end || inner[2] == end;
}

[[nodiscard]] auto Window::last_idx() const -> size_t {
    for (size_t idx = 0; idx < 3; ++idx) {
        if (inner[idx] == end) {
            return idx - 1;
        }
    }

    return 2;
}

Peephole::Peephole(
    std::vector<Instruction>& instructions,
    OptimizationFlags flags
) :
    instructions(instructions),
    optimize_offload(flags[Optimization::PeepholeOffload]),
    optimize_const_sink(flags[Optimization::PeepholeConstsink]) {}

void Peephole::apply_removal() {
    auto removal = std::remove_if(
        instructions.begin(),
        instructions.end(),
        [](Instruction inst) { return inst.opcode == Opcode::None; }
    );

    instructions.erase(removal, instructions.end());
}

void Peephole::run() {
    if (!(optimize_offload || optimize_const_sink) || instructions.size() < 2) {
        return;
    }

    Window window(instructions.begin().base(), instructions.end().base());
    run_helper(window);

    apply_removal();
}

[[nodiscard]] auto Window::branches_end_with_literals() const -> bool {
    size_t idx = true_branch->last_idx();
    if (idx < 3) {
        if (!true_branch->inner[idx]->is_literal()) {
            return false;
        }
    } else if (true_branch->true_branch
               && !true_branch->branches_end_with_literals()) {
        return false;
    }

    idx = false_branch->last_idx();
    if (idx < 3) {
        if (!false_branch->inner[idx]->is_literal()) {
            return false;
        }
    } else if (false_branch->false_branch
               && !false_branch->branches_end_with_literals()) {
        return false;
    }

    return true;
}

auto Window::remove_literals(int offset) -> int {
    int true_offset = 0;
    int false_offset = 0;

    size_t idx = true_branch->last_idx();
    if (idx < 3) {
        true_branch->inner[idx]->opcode = Opcode::None;
        true_offset = 1;
    } else if (true_branch->true_branch) {
        true_offset = true_branch->remove_literals(offset);
    }
    true_branch->root->parameter -= true_offset + offset;

    idx = false_branch->last_idx();
    if (idx < 3) {
        false_branch->inner[idx]->opcode = Opcode::None;
        false_offset = 1;
    } else if (false_branch->false_branch) {
        false_offset = false_branch->remove_literals(offset + true_offset);
    }
    false_branch->root->parameter -= true_offset + false_offset + offset;

    return true_offset + false_offset;
}

void Peephole::run_helper(Window& window) {
    while (true) {
        if (window.last_idx() == 0) {
            return;
        }

        if (window.inner[0]->opcode == Opcode::JumpFalse) {
            // this is so cursed

            auto jmp = &instructions[window.inner[0]->parameter - 1];
            auto continuation = &instructions[jmp->parameter];

            window.true_branch = std::make_unique<Window>(window.inner[1], jmp);
            window.true_branch->root = window.inner[0];
            window.true_branch->offset = window.offset;
            run_helper(*window.true_branch);

            window.false_branch =
                std::make_unique<Window>(jmp + 1, continuation);
            window.false_branch->root = jmp;
            window.false_branch->offset = window.true_branch->offset;
            run_helper(*window.false_branch);

            window.offset = window.false_branch->offset;

            if (optimize_const_sink && continuation->opcode == Opcode::Pop
                && follow_jumps(instructions, window.root) != continuation
                && window.branches_end_with_literals()) {
                window.offset += window.remove_literals(0) + 1;
                continuation->opcode = Opcode::None;
            }

            window.inner[0]->parameter -= window.false_branch->offset;
            jmp->parameter -= window.true_branch->offset;
            window.inner[0] = continuation;
            window.inner[1] = continuation + 1;
            window.inner[2] = continuation + 2;

            if (follow_jumps(instructions, continuation)->opcode
                != Opcode::Pop) {
                window.true_branch.reset();
                window.false_branch.reset();
            }
        }

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
}

auto follow_jumps(std::vector<Instruction>& instructions, Instruction* start)
    -> Instruction* {
    while (start->opcode == Opcode::Jump) {
        start = &instructions[start->parameter];
    }

    return start;
}

} // namespace dgeval::ast
