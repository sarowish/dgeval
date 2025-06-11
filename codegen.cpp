#include "codegen.hpp"
#include <sys/mman.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <variant>
#include "ast.hpp"
#include "linear_ir.hpp"
#include "runtime_library.hpp"

std::array<Register, 4> Codegen::registers =
    {Register::RDI, Register::RSI, Register::RDX, Register::RCX};

Codegen::~Codegen() {
    free(code_base);
}

void Codegen::emit_bytes(std::initializer_list<uint8_t> bytes) {
    for (auto byte : bytes) {
        emit_code_fragment(byte);
    }
}

template<typename T>
void Codegen::emit_code_fragment(T code_fragment) {
    size_t fragment_size = sizeof(code_fragment);

    if (code_len + fragment_size >= bag_size) {
        size_t new_size = bag_size + DELTA + fragment_size;
        uint8_t* new_base = (uint8_t*)realloc(code_base, new_size);

        if (new_base) {
            code_base = new_base;
            bag_size = new_size;
        }
    }

    if (code_len < bag_size) {
        *(T*)(code_base + code_len) = code_fragment;
        code_len += fragment_size;
    }
}

void* Codegen::create_code_base() {
    int page_size = getpagesize();
    size_t page_count = (code_len + page_size - 1) / page_size;
    size_t alloc_size = page_size * page_count;

    void* p = aligned_alloc(page_size, alloc_size);

    memmove(p, code_base, code_len);
    mprotect(p, alloc_size, PROT_EXEC);

    return p;
}

void Codegen::emit_prologue(int variable_count) {
    int variable_area = variable_count << 3;

    // emit_bytes({0xc8});
    // emit_code_fragment((uint16_t)variable_area);
    // emit_bytes({0});

    emit_bytes({0x55, 0x48, 0x89, 0xe5, 0x48, 0x81, 0xec});
    emit_code_fragment(variable_area);

    emit_bytes({0x41, 0x54});
}

void Codegen::emit_epilogue() {
    // emit_bytes({0x41, 0x5c, 0xc9, 0xc3});

    emit_bytes({0x41, 0x5c, 0x48, 0x89, 0xec, 0x5d, 0xc3});

    unwind_location = code_len;
    setup_immediate_integral_arg(0, (uint64_t)&runtime);
    emit_call((void*)lib::Runtime::post_exec_cleanup);
    emit_bytes({0x48, 0x89, 0xec, 0x5d, 0xc3});

    for (auto f : unwind_fixups) {
        *(uint32_t*)(code_base + f) = unwind_location - f - 4;
    }
}

void Codegen::xmm_arith_instruction(uint8_t critical_byte) {
    emit_bytes(
        {0xF2,
         0x0F,
         0x10,
         0x44,
         0x24,
         0x08,
         0xF2,
         0x0F,
         critical_byte,
         0x04,
         0x24}
    );

    emit_bytes({0x48, 0x83, 0xC4, 0x08, 0xF2, 0x0F, 0x11, 0x04, 0x24});
}

void Codegen::comparison_instruction(
    TypeDescriptor type_desc,
    uint8_t critical_byte
) {
    if (type_desc.dimension > 0) {
        setup_argument(1, false);
        setup_argument(0, false);
        emit_call((void*)lib::Runtime::arrcmp);
        emit_bytes({0x48, 0x31, 0xc9});
        emit_bytes({0x48, 0x83, 0xf8, 0x00});
    } else {
        switch (type_desc.type) {
            case Type::Number:
                emit_bytes({0x48, 0x31, 0xc9});
                emit_bytes(
                    {0x48,
                     0x83,
                     0xC4,
                     0x10,
                     0xF2,
                     0x0F,
                     0x10,
                     0x44,
                     0x24,
                     0xF8,
                     0x66,
                     0x0F,
                     0x2F,
                     0x44,
                     0x24,
                     0xF0}
                );

                if (critical_byte > 0x78) {
                    critical_byte = ((critical_byte << 1) & 0b100) | 0b10
                        | (critical_byte & (uint8_t)0xf1);
                }
                break;
            case Type::Boolean:
                emit_bytes({0x48, 0x31, 0xc9});
                emit_bytes({0x5f, 0x58, 0x48, 0x39, 0xf8});
                break;
            case Type::String:
                setup_argument(1, false);
                setup_argument(0, false);
                emit_call((void*)lib::Runtime::strcmp);
                emit_bytes({0x48, 0x31, 0xc9});
                emit_bytes({0x48, 0x83, 0xf8, 0x00});
            default:
                break;
        }
    }

    emit_bytes({critical_byte, 0x03, 0x48, 0xff, 0xc1, 0x51});
}

void Codegen::emit_call(void* call_address) {
    emit_bytes({0x48, 0xb8});
    emit_code_fragment(call_address);
    emit_bytes(
        {0x49, 0x89, 0xE4, 0x48, 0x83, 0xE4, 0xF0, 0xFF, 0xD0, 0x4C, 0x89, 0xE4}
    );
}

void Codegen::setup_argument(int idx, bool is_double) {
    emit_bytes({0x58});

    if (is_double) {
        uint8_t critical_byte = 0xc0 + 8 * idx;
        emit_bytes({0x66, 0x48, 0x0f, 0x6e, critical_byte});
    } else {
        uint8_t critical_byte = 0xc0 + std::to_underlying(registers[idx]);
        emit_bytes({0x48, 0x89, critical_byte});
    }
}

void Codegen::setup_immediate_integral_arg(int idx, uint64_t arg) {
    uint8_t critical_byte = 0xb8 + std::to_underlying(registers[idx]);
    emit_bytes({0x48, critical_byte});
    emit_code_fragment(arg);
}

void Codegen::setup_immediate_double_arg(int idx, double arg) {
    uint8_t critical_byte = 0xc0 + 8 * std::to_underlying(registers[idx]);
    emit_bytes({0x48, 0xb8});
    emit_code_fragment(arg);
    emit_bytes({0x66, 0x48, 0x0f, 0x6e, critical_byte});
}

void Codegen::place_result_on_stack(bool is_double) {
    if (is_double) {
        emit_bytes({0x66, 0x48, 0x0F, 0x7E, 0xC0});
    }

    emit_bytes({0x50});
}

void Codegen::translate_instruction(Instruction& instruction) {
    instruction.code_offset = code_len;

    switch (instruction.opcode) {
        case Opcode::Assign:
            emit_bytes({0x48, 0x8b, 0x04, 0x24, 0x48, 0x89, 0x85});
            emit_code_fragment((uint32_t)(instruction.parameter + 1) * -8);
            break;
        case Opcode::Equal:
            comparison_instruction(instruction.type, 0x75);
            break;
        case Opcode::NotEqual:
            comparison_instruction(instruction.type, 0x74);
            break;
        case Opcode::Less:
            comparison_instruction(instruction.type, 0x7d);
            break;
        case Opcode::LessEqual:
            comparison_instruction(instruction.type, 0x7f);
            break;
        case Opcode::Greater:
            comparison_instruction(instruction.type, 0x7e);
            break;
        case Opcode::GreaterEqual:
            comparison_instruction(instruction.type, 0x7c);
            break;
        case Opcode::Add:
            xmm_arith_instruction(0x58);
            break;
        case Opcode::Subtract:
            xmm_arith_instruction(0x5c);
            break;
        case Opcode::Multiply:
            xmm_arith_instruction(0x59);
            break;
        case Opcode::Divide:
            xmm_arith_instruction(0x5e);
            break;
        case Opcode::Pop:
            emit_bytes({0x48, 0x81, 0xc4});
            emit_code_fragment((uint32_t)instruction.parameter * 8);
            break;
        case Opcode::And:
            emit_bytes({0x58, 0x48, 0x21, 0x04, 0x24});
            break;
        case Opcode::Or:
            emit_bytes({0x58, 0x48, 0x09, 0x04, 0x24});
            break;
        case Opcode::Minus:
            emit_bytes({0x48, 0xb8});
            emit_code_fragment(0x8000000000000000);
            emit_bytes({0x48, 0x31, 0x04, 0x24, 0xf2, 0x0f, 0x10, 0x04, 0x24});
            break;
        case Opcode::Not:
            emit_bytes({0xb8});
            emit_code_fragment((uint32_t)1);
            emit_bytes({0x48, 0x31, 0x04, 0x24, 0xf2, 0x0f, 0x10, 0x04, 0x24});
            break;
        case Opcode::Call: {
            int double_count = 0;
            int integral_count = 0;
            FunctionSignature const& func_sig =
                RUNTIME_LIBRARY.at(get<std::string>(instruction.value));

            for (auto parameter : func_sig.parameters) {
                if (parameter == NUMBER) {
                    ++double_count;
                } else {
                    ++integral_count;
                }
            }

            if (func_sig.return_type == STRING) {
                ++integral_count;
            }

            for (int i = func_sig.parameter_count; i > 0; --i) {
                bool is_double = func_sig.parameters[i - 1] == NUMBER;

                setup_argument(
                    is_double ? (--double_count) : (--integral_count),
                    is_double
                );
            }

            if (func_sig.return_type == STRING) {
                setup_immediate_integral_arg(0, (uint64_t)&runtime);
            }

            emit_call(func_sig.entry_point);
            place_result_on_stack(func_sig.return_type == NUMBER);
            break;
        }
        case Opcode::Jump:
            emit_bytes({0xe9, 0x00, 0x00, 0x00, 0x00});
            break;
        case Opcode::JumpFalse:
            emit_bytes(
                {0x58, 0x48, 0x09, 0xc0, 0x0f, 0x84, 0x00, 0x00, 0x00, 0x00}
            );
            break;
        case Opcode::Identifier:
            emit_bytes({0xff, 0xb5});
            emit_code_fragment((uint32_t)(instruction.parameter + 1) * -8);
            break;
        case Opcode::Literal:
            if (std::holds_alternative<double>(instruction.value)) {
                emit_bytes({0x48, 0xb8});
                emit_code_fragment(get<double>(instruction.value));
                emit_bytes({0x50});
            } else if (std::holds_alternative<bool>(instruction.value)) {
                emit_bytes({0x6a, get<bool>(instruction.value)});
            }
            break;
        case Opcode::CallLRT:
            switch (instruction.parameter) {
                case 0: {
                    emit_bytes({0x48, 0x89, 0xe1});

                    int item_count = get<double>(instruction.value);
                    uint64_t type_desc = *(uint64_t*)&instruction.type;

                    setup_immediate_integral_arg(2, item_count);
                    setup_immediate_integral_arg(1, type_desc);
                    setup_immediate_integral_arg(0, (uint64_t)&runtime);

                    emit_call((void*)lib::Runtime::allocate_array);

                    emit_bytes({0x48, 0x81, 0xc4});
                    emit_code_fragment((uint32_t)(item_count * 8));

                    place_result_on_stack(false);
                } break;
                case 1:
                    setup_argument(0, true);
                    emit_bytes({0xf2, 0x48, 0x0f, 0x2d, 0xd0});

                    setup_argument(1, false);
                    setup_immediate_integral_arg(0, (uint64_t)&runtime);
                    emit_call((void*)lib::Runtime::array_element);
                    place_result_on_stack(false);

                    setup_immediate_integral_arg(0, (uint64_t)&runtime);
                    emit_call((void*)lib::Runtime::check_exception);

                    emit_bytes({0x48, 0x09, 0xc0});

                    unwind_fixups.push_back(code_len + 2);
                    emit_bytes({0x0f, 0x85, 0, 0, 0, 0});
                    break;
                case 2:
                    setup_argument(1, false);
                    setup_argument(0, false);
                    emit_call((void*)lib::Runtime::append_element);
                    place_result_on_stack(false);
                    break;
                case 3:
                    setup_immediate_integral_arg(
                        1,
                        (uint64_t)&get<std::string>(instruction.value)
                    );
                    setup_immediate_integral_arg(0, (uint64_t)&runtime);
                    emit_call((void*)lib::Runtime::allocate_string);
                    place_result_on_stack(false);
                    break;
                case 4:
                    setup_argument(2, false);
                    setup_argument(1, false);
                    setup_immediate_integral_arg(0, (uint64_t)&runtime);
                    emit_call((void*)lib::Runtime::cat_string);
                    place_result_on_stack(false);
                    break;
                case 5:
                    setup_argument(0, true);
                    setup_immediate_integral_arg(0, (uint64_t)&runtime);
                    emit_call((void*)lib::Runtime::number_to_string);
                    place_result_on_stack(false);
                    break;
                case 6: {
                    int64_t comparison = get<bool>(instruction.value);
                    setup_immediate_integral_arg(2, comparison);
                    setup_argument(1, false);
                    setup_argument(0, false);
                    emit_call((void*)lib::Runtime::strcmp);
                    place_result_on_stack(false);
                } break;
                case 7: {
                    setup_argument(1, false);
                    setup_argument(0, false);
                    emit_call((void*)lib::Runtime::arrcmp);
                    place_result_on_stack(false);
                } break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

void Codegen::backpatch_instructions(std::vector<Instruction>& instructions) {
    for (size_t idx = 0; idx < instructions.size(); ++idx) {
        auto& instruction = instructions[idx];

        if (instruction.opcode == Opcode::Jump
            || instruction.opcode == Opcode::JumpFalse) {
            int target = instructions[instruction.parameter].code_offset;
            int next_offset = instructions[idx + 1].code_offset;
            *(uint32_t*)(code_base + next_offset - 4) = target - next_offset;
        }
    }
}

DynamicFunction* Codegen::run(Program& program) {
    emit_prologue(program.symbol_table.size());

    for (auto& instruction : program.instructions) {
        translate_instruction(instruction);
    }

    emit_epilogue();
    backpatch_instructions(program.instructions);

    return (DynamicFunction*)create_code_base();
}
