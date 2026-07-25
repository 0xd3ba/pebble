#pragma once

#include "primitives/register.hpp"

namespace pebble::primitives {

/* Latch<T> - a pipeline register between two stages, built from two
 * Register<T> instances (input side, output side).
 *
 * Models real flip-flop/pipeline-latch behavior: whatever a stage writes
 * via set_input() this cycle is NOT visible via get_output() until
 * advance() fires (the clock edge). This prevents a same-cycle
 * read-after-write across a latch boundary, which would model an
 * impossible combinational loop rather than a real pipeline register.
 * A stage that wants to hold data across a stall cycle must explicitly re-invoke set_input() every cycle
 */
template<typename T>
class Latch {
public:
    Latch() = default;

    void set_input(T value) {
        input_.write(value);
        input_set_this_cycle = true;
    }

    /* Clock edge: commit this cycle's input (if any) to the output side.
     * No input set this cycle -> output becomes a bubble (invalid) */
    void advance() {
        if(input_set_this_cycle)
            output_.write(input_.read());
        else
            output_.invalidate();

        input_.invalidate();
        input_set_this_cycle = false;
    }

    const T& get_output() const { return output_.read(); }
    bool has_output() const noexcept { return output_.valid(); }

    /* Pipeline flush -- invalidate whatever is in this latch right now */
    void squash() {
        input_.invalidate();
        output_.invalidate();
        input_set_this_cycle = false;
    }

private:
    Register<T> input_;
    Register<T> output_;
    bool input_set_this_cycle{false};
};

}  // namespace pebble::primitives