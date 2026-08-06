#pragma once

#include <string>
#include "loader/elf_image.hpp"

namespace pebble::loader {

/* Loader -- parses a statically-linked, bare-metal ELF binary (via the ELFIO library) into an ElfImage */
class Loader {
public:
    /* Throws std::runtime_error on any parse failure: file doesn't
     * exist, isn't a valid ELF, isn't 32-bit RISC-V, or isn't a
     * statically-linked executable (no support for dynamic linking) */
    [[nodiscard]] static ElfImage load(const std::string &elf_path);
};

}  // namespace pebble::loader