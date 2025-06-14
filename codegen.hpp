#pragma once

#include <cstdint>
#include <initializer_list>
#include "context.hpp"
#include "lang_runtime.hpp"

using dgeval::ast::FunctionSignature;
using dgeval::ast::Instruction;
using dgeval::ast::NUMBER;
using dgeval::ast::Opcode;
using dgeval::ast::Program;
using dgeval::ast::RUNTIME_LIBRARY;
using dgeval::ast::STRING;
using dgeval::ast::Type;
using dgeval::ast::TypeDescriptor;
using std::size_t;

const int DELTA = 16;

enum class Register : uint8_t { RAX, RCX, RDX, RBX, RSP, RBP, RSI, RDI };

using DynamicFunction = void();

class Codegen {
  public:
    ~Codegen();
    void emit_bytes(std::initializer_list<uint8_t> bytes);
    template<typename T>
    void emit_code_fragment(T code_fragment);
    [[nodiscard]] auto create_code_base() const -> void*;
    void emit_prologue(int variable_count);
    void emit_epilogue();
    void xmm_arith_instruction(uint8_t critical_byte);
    void
    comparison_instruction(TypeDescriptor type_desc, uint8_t critical_byte);
    void setup_argument(int idx, bool is_double);
    void emit_call(void* call_address);
    void setup_immediate_integral_arg(int idx, uint64_t arg);
    void setup_immediate_double_arg(int idx, double arg);
    void place_result_on_stack(bool is_double);
    void translate_function_call(Instruction& instruction);
    void translate_lrt(Instruction& instruction);
    void translate_instruction(Instruction& instruction);
    void backpatch_instructions(std::vector<Instruction>& instructions) const;
    auto generate(Program& program) -> DynamicFunction*;

    lib::Runtime runtime;
    static std::array<Register, 4> registers;
    uint8_t* code_base {std::bit_cast<uint8_t*>(malloc(DELTA))};
    size_t bag_size {DELTA};
    size_t code_len {0};
    size_t unwind_location;
    std::vector<int> unwind_fixups;
};
