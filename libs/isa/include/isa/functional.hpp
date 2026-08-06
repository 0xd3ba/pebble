#pragma once

#include <optional>
#include <string>
#include <spdlog/spdlog.h>
#include "isa/addr.hpp"
#include "isa/arf.hpp"
#include "isa/csrf.hpp"
#include "isa/decoder.hpp"
#include "isa/disassembler.hpp"
#include "isa/flat_memory.hpp"
#include "isa/instruction.hpp"
#include "isa/mem_width.hpp"
#include "isa/ops.hpp"
#include "isa/reg_id.hpp"
#include "isa/trap.hpp"
#include "utils/tracer.hpp"

namespace pebble::isa::functional {

/* FunctionalExecutionResult -- everything that execute(...) computes for one instruction,
 * reported as data rather than applied as a side effect.
 *
 * rd/writeback_value: what to write back to the register file. rd is
 * nullopt for instructions with no destination register (stores,
 * branches etc.) -- writeback_value is meaningless in
 * that case and should be ignored by the caller rather than relied on.
 *
 * mem_addr/store_value: deferred memory read/write. mem_addr is nullopt
 * for every non-memory instruction; when present, FunctionalCpu::step()
 * performs the actual FlatMemory::read()/write() at mem-access/writeback (commit).
 *
 * next_pc: set only when control flow diverges from the default
 * PC+4 (taken branches, JAL, JALR). FunctionalCpu falls back to PC+4
 * itself when this is nullopt -- execute() never computes PC+4 for the
 * non-branching case, to keep "what changed" honestly minimal.
 *
 * trap: Trap::none() on ordinary successful execution. Set for illegal
 * instructions (though in practice Decoder already caught those before
 * execute(...) is even called). Memory access/alignment faults propagated up from FlatMemory, and the
 * ecall/ebreak system instructions, which are modeled purely as traps with no other effects */
struct FunctionalExecutionResult {
    std::optional<RegId> rd;
    arf_t writeback_value{0};

    std::optional<addr_t> mem_addr;
    word_t store_value{0};

    std::optional<uint16_t> csr_addr;
    uint32_t csr_value{0};

    std::optional<addr_t> next_pc;

    Trap trap{};

    [[nodiscard]] static FunctionalExecutionResult none() { return FunctionalExecutionResult{}; }
};

/* FunctionalExecutionTrace -- anything that will be needed for debugging when something goes wrong */
struct FunctionalExecutionTrace {
    uint64_t cycle;
    addr_t pc;
    std::optional<Instruction> instr;
};

/* execute() -- Pure computation of one decoded instruction without committing the result
 * Note: CsrFile is passed for uniformity across all instruction families for consistency, even though only
 * ecall/ebreak instructions use it */
[[nodiscard]] FunctionalExecutionResult execute(const Instruction& instr, addr_t pc, const ArchRegisterFile& regs, CsrFile &csrf);

/* FunctionalCpu -- single-cycle, non-pipelined RV32I+M interpreter.
 * Each step() call fully commits exactly one instruction: fetch, decode,
 * execute, mem-access, writeback, PC update, CSRF bookkeeping.
 * No timing, no speculation, no overlap between instructions.
 * Mainly built to be used as comparison for functional correctness against other (real, timing-modeling) CPU implementations
 *
 * IMPORTANT: step() returns a Trap rather than throwing or maintaining an internal "halted" flag. On a failed execution,
 * the caller needs to halt execution; the PC is not updated in FunctionalCpu, not doing so would lead to an infinite loop */
class FunctionalCpu {
public:
    FunctionalCpu() = delete;
    FunctionalCpu(addr_t pc, ArchRegisterFile &regs, FlatMemory &mem, CsrFile &csrf):
        pc_{pc}, regs_{regs}, mem_{mem}, csrf_{csrf}
    {}

    FunctionalCpu(const FunctionalCpu &other) = delete;
    FunctionalCpu& operator=(const FunctionalCpu &other) = delete;

    [[nodiscard]] Trap step() {
        FunctionalExecutionTrace trace{};
        trace.cycle = csrf_.read_cycle();
        trace.pc = pc_;

        // stage-1: instruction fetch
        flat_memory::ReadResult read_res = mem_.read(pc_, MemWidth::Word);
        if(read_res.trap.is_trap()) return finish_with_trap(read_res.trap, trace);

        // stage-2: instruction decode
        word_t word = read_res.value;
        Instruction instr = Decoder::decode(word);
        trace.instr = instr;
        if(instr.is_illegal()) {
            Trap t{};
            t.kind = TrapKind::IllegalInstruction;
            t.message = "illegal instruction encoding: " + std::to_string(instr.raw);
            return finish_with_trap(t, trace);
        }

        /* stage-3: (register read +) execute
         * register read normally happens alongside decode; decode(...) designed specifically to only
         * decode an instruction (single responsibility). In either case, register read happens before execution */
        FunctionalExecutionResult result = execute(instr, pc_, regs_, csrf_);
        if(result.trap.is_trap()) finish_with_trap(result.trap, trace);

        // stage-4: memory-access
        if(instr.op_fam == OpFamily::Load || instr.op_fam == OpFamily::Store) {
            addr_t mem_addr = *result.mem_addr;

            // case: load
            if(instr.op_fam == OpFamily::Load) {
                flat_memory::ReadResult read_res = mem_.read(mem_addr, width_of_mem_op(instr.op));
                if(read_res.trap.is_trap()) return finish_with_trap(read_res.trap, trace);
                // store the value in the result's writeback field
                result.writeback_value = format_load_value(instr.op, read_res.value);
            }

            // case: store
            else {
                Trap t = mem_.write(mem_addr, width_of_mem_op(instr.op), result.store_value);
                if(t.is_trap()) return finish_with_trap(t, trace);
            }
        }

        // stage-5: writeback (commit)
        if(result.rd.has_value()) regs_.write(*result.rd, result.writeback_value);

        // update PC and other bookkeeping information
        pc_ = result.next_pc.value_or(pc_ + 4);
        csrf_.increment_cycle();
        csrf_.increment_instret();
        if(result.csr_addr.has_value())
            csrf_.write(*result.csr_addr, result.csr_value);

        tracer_.push_back(trace);
        return Trap::none();
    }

    /* Dump the execution trace */
    void dump_trace() {
        spdlog::info("dumping execution trace (oldest -> newest): [cycle] [pc] [raw_instruction] [decoded instruction]");
        tracer_.for_each([](const FunctionalExecutionTrace &trace) {
            bool instr_valid = trace.instr.has_value();
            std::string fmt_instr = instr_valid? debug::Disassembler::disassemble(*trace.instr): "";
            word_t raw_instr = instr_valid? trace.instr->raw: 0;
            spdlog::info("0x{:016x} 0x{:08x} 0x{:08x} {}", trace.cycle, trace.pc, raw_instr, fmt_instr);
        });
    }

private:
    addr_t pc_;
    ArchRegisterFile &regs_;
    FlatMemory &mem_;
    CsrFile &csrf_;
    utils::Tracer<FunctionalExecutionTrace, 64> tracer_{};

    Trap finish_with_trap(Trap &t, FunctionalExecutionTrace trace) {
        if(!t.faulting_addr.has_value()) t.faulting_addr = pc_;
        csrf_.increment_cycle();
        csrf_.set_mepc(pc_);
        csrf_.set_mcause(t.kind);
        // instret is deliberately NOT incremented: the trapping instruction did not retire/commit to architectural state
        tracer_.push_back(std::move(trace));
        return t;
    }

};

}  // namespace pebble::isa
