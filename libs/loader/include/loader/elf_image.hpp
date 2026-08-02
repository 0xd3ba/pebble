#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace pebble::loader {

/* Segment -- one PT_LOAD segment's raw content, exactly as ELFIO provides.
 * Note: PT_LOAD segments are the subset of program headers that actually need to be copied into
 * memory to run the program */
struct Segment {
    uint64_t vaddr{0};                  // where in the address space
    uint64_t memsz{0};                  // how many bytes it occupies in memory (often larger than file_bytes.size(): uninitialized bytes not present in file_bytes)
    std::vector<uint8_t> file_bytes{};  // actual bytes stored in ELF binary. file_bytes.size(): how many bytes exist in the file
};

/* ElfImage -- everything that Loader extracted from a parsed ELF binary */
struct ElfImage {
    std::vector<Segment> segments;  // every loadable segment
    uint64_t entry_point{0};        // e_entry in ELF header -- the address execution should start at

    /* Contains every symbol from the ELF binary: global/static variables, function names etc. (local variables not present)
     * Basically a symbol -> virtual address mapping */
    std::unordered_map<std::string, uint64_t> symbols_map;

    /* Highest address any segment reaches (vaddr + memsz, maximized across segments). Every CPU model's harness needs this
     * number to size its own memory before placing segments into it */
    [[nodiscard]] uint64_t highest_extent() const {
        uint64_t highest = 0;
        for(const auto& s: segments) highest = std::max(highest, s.vaddr + s.memsz);
        return highest;
    }

    /* Returns the virtual address of the symbol, if it exists; std:nullopt otherwise */
    [[nodiscard]] std::optional<uint64_t> find_symbol(const std::string &symbol) {
        auto it = symbols_map.find(symbol);
        if(it == symbols_map.end()) return std::nullopt;
        return it->second;
    }
};

}  // namespace pebble::loader