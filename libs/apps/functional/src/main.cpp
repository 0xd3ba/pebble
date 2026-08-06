#include <cstdint>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <spdlog/spdlog.h>
#include "isa/addr.hpp"
#include "isa/arf.hpp"
#include "isa/csrf.hpp"
#include "isa/flat_memory.hpp"
#include "isa/functional.hpp"
#include "isa/mem_width.hpp"
#include "loader/elf_image.hpp"
#include "loader/loader.hpp"
#include "utils/argparse.hpp"
#include "utils/cast.hpp"

namespace pisa = pebble::isa;
namespace pldr = pebble::loader;
namespace putils = pebble::utils;

namespace {

constexpr uint64_t kMaxInstructions = 1'000'000;    // hard upper-bound on instruction executions (unless overridden via CLI)
constexpr uint64_t kMaxStackBytes = (1u << 12);     // hard upper-bound on call stack size (unless overridden via cli)
constexpr uint8_t rv_stk_reg_id = 2;                // x2 register holds the stack pointer according to risc-v abi
constexpr std::string_view rv_tohost = "tohost";    // special symbol used by risc-v tests to communicate the execution status

/* Loads an ElfImage's segments into a fresh FlatMemory and sets up the
 * initial stack pointer + entry PC.
 * Note: This is the only specific for FunctionalCpu -- other cpu models might need their own separate initialization step */
struct Program {
    pisa::FlatMemory mem;
    pisa::ArchRegisterFile regs;
    pisa::CsrFile csrf;
    pldr::vaddr_t entry_pc;
    std::optional<pldr::vaddr_t> tohost_addr;  // optional since risc-v abi doesn't mandate others to have a "tohost" symbol
};

/* Initializes the flat-memory with the PT_LOAD segments and the stack pointer (SP) */
Program load(const pldr::ElfImage image, uint64_t stk_bytes) {
    pldr::vaddr_t mem_req = image.highest_extent() + stk_bytes;
    Program prgm{
        .mem = pisa::FlatMemory{mem_req},
        .regs = pisa::ArchRegisterFile{},
        .csrf = pisa::CsrFile{},
        .entry_pc = image.entry_point,
        .tohost_addr = image.find_symbol(std::string{rv_tohost})
    };

    // write the bytes of each segment to flat memory
    for(const auto &s: image.segments) {
        for(std::size_t i=0; i<s.file_bytes.size(); i++) {
            pldr::vaddr_t vaddr = s.vaddr;
            auto t = prgm.mem.write(static_cast<pisa::addr_t>(vaddr)+i, pisa::MemWidth::Byte ,s.file_bytes[i]);
            // shouldn't happen unless there is a bug with the flat memory implementation or the provisioned memory
            if(t.is_trap())
                throw std::runtime_error{"error during flatmemory write: " + t.message};
        }
        spdlog::info("load: {} bytes from segment written to flat-memory", s.file_bytes.size());
    }

    // stack grows downwards and initial pointer must be 16-byte aligned according to risc-v abi
    const uint32_t stk_top = putils::Cast::u32(mem_req) & ~putils::Cast::u32(0xf);
    prgm.regs.write(pisa::RegId{rv_stk_reg_id}, stk_top);

    return prgm;
}

/* Runs the program to completion, printing the outcome and returning a
 * process exit code (0 = pass, 1 = fail/crash/timeout). Two ways a
 * normal program will end:
 *     - it writes riscv-test's "tohost" completion word (test-suite-specific)
 *     - maximum # of instructions have been executed
 *     - unhandled trap/error (illegal instruction etc.) */
int run(Program &prgm, uint64_t max_cycles) {
    pisa::addr_t entry_pc = static_cast<pisa::addr_t>(prgm.entry_pc);
    pisa::functional::FunctionalCpu cpu{entry_pc, prgm.regs, prgm.mem, prgm.csrf};

    spdlog::info("starting simulation");
    for(uint64_t cycle=0; cycle<max_cycles; cycle++) {
        auto trap = cpu.step();  // execute one step

        // poll the tohost address after every step -- "are we done yet?"
        if(prgm.tohost_addr.has_value()) {
            auto read = prgm.mem.read(static_cast<pisa::addr_t>(*prgm.tohost_addr), pisa::MemWidth::Word);
            if(!read.trap.is_trap() && read.value != 0) {
                const int32_t exit_code = read.value >> 1;
                /* in case of failure, (value >> 1) holds the failed test number. (value & 0x1) is always 1 as
                 * bit0 is always set (other bits only set on failure): exit_code==0 means test passed */
                std::string_view status = exit_code? "FAIL": "PASS";
                spdlog::info("[{}] cycles={} exit_code={}", std::string{status}, cycle, exit_code);
                return exit_code? -1: 0;
            }
        }

        if(trap.is_trap()) {
            // specific to Linux ABI (risc-v tests also seem to signal exit via writing value of 93/94 to x17 register)
            if(trap.kind == pisa::TrapKind::EnvironmentCallFromMMode) {
                pisa::arf_t a17 = prgm.regs.read(pisa::RegId{17});  // value of 93/94 signals exit
                pisa::arf_t a10 = prgm.regs.read(pisa::RegId{10});  // stores the exit code

                if(a17 != 93 && a17 != 94) continue;
                std::string_view status = (a10 != 0)? "FAIL": "PASS";
                spdlog::info("[{}] cycles={} exit_code={}", std::string{status}, cycle, a10);
                return a10? -1: 0;
            }

            // unhandled exception -- halt immediately
            spdlog::critical("[CRASH] cycles={} at={} reason={}", cycle, *trap.faulting_addr, trap.message);
            cpu.dump_trace();
            return -1;
        }
    }

    spdlog::info("[TIMEOUT] cycles={}", max_cycles);
    return 0;
}

}  // namespace

int main(int argc, char **argv) {
    std::string elf_path;
    uint64_t max_instructions = kMaxInstructions;
    uint64_t max_stk_bytes = kMaxStackBytes;

    putils::ArgumentParser arg_parser{"functional cpu"};
    arg_parser
        .add_required(putils::ArgumentOptions::kExecutablePath, elf_path)
        .add_option(putils::ArgumentOptions::kMaxInstructions, max_instructions)
        .add_option(putils::ArgumentOptions::kMaxStackBytes, max_stk_bytes);

    if(!arg_parser.parse(argc, argv))
        return arg_parser.exit_code();

    try {
        auto img = pldr::Loader::load(elf_path);
        auto prgm = load(img, max_stk_bytes);
        return run(prgm, max_instructions);  // single-cycle cpu; 1 cycle = 1 instruction
    } catch(const std::exception &err) {
        spdlog::critical(err.what());
        return -1;
    }
}
