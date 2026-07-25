#pragma once

namespace pebble::primitives {

/* Clockable -- the contract every stateful, cycle-driven module in the
 * simulator implements. A top-level owner calls tick() on every registered
 * Clockable once per cycle, in a fixed, deterministic order
 *
 * reset() restores the module to its power-on/initial state: used at
 * simulator startup and potentially for warm-reset scenarios in testing
 */
class Clockable {
public:
    virtual ~Clockable() = default;

    virtual void tick() = 0;
    virtual void reset() = 0;
};

}  // pebble::primitives