#pragma once

#include <optional>
#include "isa/addr.hpp"
#include "isa/arf.hpp"
#include "isa/csrf.hpp"
#include "isa/flat_memory.hpp"
#include "isa/instruction.hpp"
#include "isa/reg_id.hpp"
#include "isa/trap.hpp"

namespace pebble::isa::functional {

/* FunctionalExecutionResult -- everything that execute(...) computes for one instruction,
 * reported as data rather than applied as a side effect.
 *
 * rd/writeback_value: what to write back to the register file. rd is
 * nullopt for instructions with no destination register (stores,
 * branches etc.) -- writeback_value is meaningless in
 * that case and should be ignored by the caller rather than relied on.
 *
 * store_addr/store_value: deferred memory write. store_addr is nullopt
 * for every non-store instruction; when present, FunctionalCpu::step()
 * performs the actual FlatMemory::write() at commit.
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

    std::optional<addr_t> store_addr;
    word_t store_value{0};

    std::optional<addr_t> next_pc;

    Trap trap{};

    [[nodiscard]] static FunctionalExecutionResult none() { return FunctionalExecutionResult{}; }
};

/* execute() -- Pure computation of one decoded instruction without committing the result
 * Note: CsrFile is passed for uniformity across all instruction families for consistency, even though only
 * ecall/ebreak instructions use it */
[[nodiscard]] FunctionalExecutionResult execute(const Instruction& instr, addr_t pc, const ArchRegisterFile& regs, const FlatMemory& mem, const CsrFile& csrf);

}  // namespace pebble::isa
