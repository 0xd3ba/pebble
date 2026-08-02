#include <string>
#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include "utils/argparse.hpp"

using namespace pebble::utils;

namespace {
/* Small helper: builds a char*[] argv from string literals so tests
 * read like real command lines rather than manually-constructed argc/argv boilerplate */
class Argv {
public:
    explicit Argv(std::vector<std::string> args) : args_{std::move(args)} {
        for (auto &a : args_)
            ptrs_.push_back(a.data());
    }

    int argc() const { return static_cast<int>(ptrs_.size()); }
    char** argv() { return ptrs_.data(); }

private:
    std::vector<std::string> args_;
    std::vector<char*> ptrs_;
};

}  // namespace

TEST(ArgumentParserTest, RequiredOptionParsesSuccessfully) {
    std::string binary;
    ArgumentParser parser{"test"};
    parser.add_required("binary", binary, "path");

    Argv args{ {"prog", "test.elf"} };
    EXPECT_TRUE(parser.parse(args.argc(), args.argv()));
    EXPECT_EQ(binary, "test.elf");
}

TEST(ArgumentParserTest, MissingRequiredOptionFailsToParse) {
    std::string binary;
    ArgumentParser parser{"test"};
    parser.add_required("binary", binary, "path");

    Argv args{ {"prog"} };
    EXPECT_FALSE(parser.parse(args.argc(), args.argv()));
    EXPECT_NE(parser.exit_code(), 0);
}

TEST(ArgumentParserTest, OptionalOptionUsesDefaultWhenNotProvided) {
    std::string binary;
    uint64_t max_instructions = 42;

    ArgumentParser parser{"test"};
    parser.add_required("binary", binary, "path");
    parser.add_option("--max-instructions", max_instructions, "budget");

    Argv args{ {"prog", "test.elf"} };
    ASSERT_TRUE(parser.parse(args.argc(), args.argv()));
    EXPECT_EQ(max_instructions, 42);
}

TEST(ArgumentParserTest, OptionalOptionOverridesDefaultWhenProvided) {
    std::string binary;
    std::uint64_t max_instructions = 42;

    ArgumentParser parser{"test"};
    parser.add_required("binary", binary, "path");
    parser.add_option("--max-instructions", max_instructions, "budget");

    Argv args{ {"prog", "test.elf", "--max-instructions", "1000"} };
    ASSERT_TRUE(parser.parse(args.argc(), args.argv()));
    EXPECT_EQ(max_instructions, 1000);
}

TEST(ArgumentParserTest, FlagDefaultsToFalse) {
    bool verbose = false;
    ArgumentParser parser{"test"};
    parser.add_flag("--verbose", verbose, "enable verbose output");

    Argv args{ {"prog"} };
    ASSERT_TRUE(parser.parse(args.argc(), args.argv()));
    EXPECT_FALSE(verbose);
}

TEST(ArgumentParserTest, FlagSetToTrueWhenProvided) {
    bool verbose = false;
    ArgumentParser parser{"test"};
    parser.add_flag("--verbose", verbose, "enable verbose output");

    Argv args{ {"prog", "--verbose"} };
    ASSERT_TRUE(parser.parse(args.argc(), args.argv()));
    EXPECT_TRUE(verbose);
}

TEST(ArgumentParserTest, HelpFlagFailsParseWithZeroExitCode) {
    ArgumentParser parser{"test"};
    Argv args{ {"prog", "--help"} };
    EXPECT_FALSE(parser.parse(args.argc(), args.argv()));
    EXPECT_EQ(parser.exit_code(), 0);  // --help is not an error
}

TEST(ArgumentParserTest, InvalidValueTypeFailsToParse) {
    uint64_t max_instructions = 0;
    ArgumentParser parser{"test"};
    parser.add_option("--max-instructions", max_instructions, "budget");

    Argv args{ {"prog", "--max-instructions", "not_a_number"} };
    EXPECT_FALSE(parser.parse(args.argc(), args.argv()));
    EXPECT_NE(parser.exit_code(), 0);
}
