#pragma once

#include <string>
#include <string_view>
#include <CLI/CLI.hpp>

namespace pebble::utils {

/* ArgumentParser -- a thin, shared wrapper around CLI11, so every CPU model's
 * test harness gets consistent --help output, error handling, and option-binding syntax
 * without each one creating its own argv parsing */
class ArgumentParser {
public:
    ArgumentParser() = default;
    explicit ArgumentParser(std::string_view description): app_{std::string{description}} {}

    /* Registers an optional named argument (e.g. "--max-instructions") binding it to `value`*/
    template<typename T>
    ArgumentParser& add_option(std::string_view name, T &value, std::string_view description) {
        app_.add_option(std::string{name}, value, std::string{description});
        return *this;
    }

    /* Registers a required named or a positional argument: parse() fails without them */
    template<typename T>
    ArgumentParser& add_required(std::string_view name, T &value, std::string_view description) {
        app_.add_option(std::string{name}, value, std::string{description})->required();
        return *this;
    }

    /* Registers a boolean flag (e.g. "--verbose"), defaulting to false unless already set otherwise in `value` */
    ArgumentParser& add_flag(std::string_view name, bool &value, std::string_view description) {
        app_.add_flag(std::string{name}, value, std::string{description});
        return *this;
    }

    /* Parses argc/argv. Returns true on success (all bound variables are now populated);
     * returns false if the user asked for --help (already printed) or gave invalid arguments (error already printed) */
     bool parse(int argc, char **argv) {
        try {
            app_.parse(argc, argv);
            return true;
        } catch(const CLI::ParseError &err) {
            last_exit_code_ = app_.exit(err);
            return false;
        }
     }

    /* Valid only after parse(...) returned false */
    int exit_code() const noexcept { return last_exit_code_; }

private:
    CLI::App app_{};
    int last_exit_code_ = 0;
};

}