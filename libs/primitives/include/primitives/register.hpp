#pragma once

#include <stdexcept>
#include <utility>

namespace pebble::primitives {

class InvalidRegisterRead: public std::logic_error {
public:
    using std::logic_error::logic_error;
};

/* Register<T>: a single-slot, validity-tracked storage cell */
template<typename T>
class Register {
public:
    Register() = default;
    explicit Register(T value): value_{std::move(value)}, valid_{true} {}

    /* Stores the new value. Becomes valid irrespective of prior state */
    void write(T value) {
        value_ = std::move(value);
        valid_ = true;
    }

    /* Reads the current value. Throws error if register state is invalid */
    [[nodiscard]] const T& read() const {
        if(!valid_) throw InvalidRegisterRead{"Register<T>::read() called while register is invalid"};
        return value_;
    }

    /* Returns validity of the register */
    [[nodiscard]] bool valid() const noexcept { return valid_; }

    /* Marks the register as invalid */
    void invalidate() noexcept { valid_ = false; }

private:
    T value_{};
    bool valid_ = false;
};

}  // namespace pebble::foundation