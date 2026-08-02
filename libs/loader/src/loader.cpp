#include <cstdint>
#include <string>
#include <stdexcept>
#include <utility>
#include <elfio/elfio.hpp>
#include "loader/elf_image.hpp"
#include "loader/loader.hpp"

namespace pebble::loader {

ElfImage Loader::load(const std::string &elf_path) {
    ELFIO::elfio reader{};
    if(!reader.load(elf_path))
        throw std::runtime_error{"loader: failed to parse elf file:" + elf_path};

    if(reader.get_class() != ELFIO::ELFCLASS32)
        throw std::runtime_error{"loader: only 32-bit executables supported"};

    if(reader.get_machine() != ELFIO::EM_RISCV)
        throw std::runtime_error{"loader: machine type not RISC-V"};

    if(reader.get_type() != ELFIO::ET_EXEC)
        throw std::runtime_error{"loader: only statically linked executables supported"};

    ElfImage elf_img{};
    elf_img.entry_point = reader.get_entry();

    // PT_LOAD segments -> Segment structs
    for(const auto &segment: reader.segments) {
        if(segment->get_type() != ELFIO::PT_LOAD) continue;
        Segment s{};
        s.vaddr = segment->get_virtual_address();
        s.memsz = segment->get_memory_size();

        const char *data = segment->get_data();
        const auto file_size = segment->get_file_size();
        s.file_bytes.assign(data, data+file_size);
        elf_img.segments.push_back(std::move(s));
    }

    if(elf_img.segments.empty())
        throw std::runtime_error{"loader: no PT_LOAD segments found"};

    /* populate the symbols map
     * reference: https://elfio.sourceforge.net/ */
    for(const auto &section: reader.sections) {
        if(section->get_type() != ELFIO::SHT_SYMTAB) continue;
        const ELFIO::symbol_section_accessor symbols{reader, section.get()};  // note: unlike what the doc says, need to pass section.get(), not section
        const auto sym_count = symbols.get_symbols_num();

        for(size_t i=0; i<sym_count; i++) {
            std::string name{};
            ELFIO::Elf64_Addr value{0};
            ELFIO::Elf_Xword size{0};
            unsigned char bind = 0;
            unsigned char type = 0;
            ELFIO::Elf_Half section_index{0};
            unsigned char other = 0;

            symbols.get_symbol(i, name, value, size, bind, type, section_index, other);
            if (name.empty()) continue;
            elf_img.symbols_map[name] = value;
        }
    }

    return elf_img;
}

}  // namespace pebble::loader