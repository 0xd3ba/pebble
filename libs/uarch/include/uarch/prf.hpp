#pragma once

#include <cstdint>
#include <vector>
#include "primitives/index.hpp"
#include "primitives/register.hpp"

namespace pebble::uarch {

/* Fixed compile-time upper-bound on the number of physical registers; actual upper-bound enforced
 * by the PhysicalRegisterFile at runtime */
constexpr std::size_t kMaxPhysRegisters = 1024;
using PhysRegId = primitives::Index<kMaxPhysRegisters>;
using prf_t = uint32_t;

/* PhysicalRegisterFile -- value storage for renamed architectural registers */
class PhysicalRegisterFile {
public:
    PhysicalRegisterFile() = delete;
    explicit PhysicalRegisterFile(std::size_t max_regs): max_regs_{max_regs}, regs_(max_regs) {
        if(max_regs_ == 0 || max_regs_ > kMaxPhysRegisters)
            throw std::invalid_argument{"PhysicalRegisterFile: max_regs must be > 0 and <= " + std::to_string(kMaxPhysRegisters)};
    }

    [[nodiscard]] prf_t read(PhysRegId id) const {
        check_bounds(id);
        return regs_[id.index()].read();  // may throw if register is not ready
    }

    void write(PhysRegId id, prf_t value) {
        check_bounds(id);
        regs_[id.index()].write(value);
    }

    bool is_ready(PhysRegId id) const {
        check_bounds(id);
        return regs_[id.index()].valid();
    }

    void invalidate(PhysRegId id) {
        check_bounds(id);
        regs_[id.index()].invalidate();
    }

private:
    std::size_t max_regs_;
    std::vector<primitives::Register<prf_t>> regs_;

    void check_bounds(PhysRegId id) const {
        if(id.index() >= max_regs_)
            throw std::invalid_argument{"PhysRegId: index out of bounds: " + std::to_string(id.index())};
    }
};

}  // namespace pebble::uarch