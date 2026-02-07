// test_cli_parser.cpp - Tests for CLI parser argv/REPL equivalence
//
// These tests verify the ADR-006 invariant:
//   argv(load import.db --verbose) == repl("load import.db --verbose")

#include "test_framework.h"
#include "../cli/parser/parser.h"
#include <cstring>

using namespace gtaf::cli;
using namespace gtaf::test;

// ---- Helper Functions ----

namespace {

/// @brief Helper to simulate argv parsing
/// @param args Space-separated arguments (will be tokenized like shell would)
Command parse_as_argv(Parser& parser, const std::vector<std::string>& args) {
    // Build argc/argv from string vector
    // argv[0] is program name, rest are arguments
    std::vector<const char*> argv_ptrs;
    argv_ptrs.push_back("gtaf"); // program name
    for (const auto& arg : args) {
        argv_ptrs.push_back(arg.c_str());
    }

    return parser.parse_argv(
        static_cast<int>(argv_ptrs.size()),
        const_cast<char**>(argv_ptrs.data())
    );
}

/// @brief Compare two Command objects for equality
bool commands_equal(const Command& a, const Command& b) {
    if (a.name != b.name) return false;
    if (a.positionals != b.positionals) return false;
    if (a.options != b.options) return false;
    if (a.flags != b.flags) return false;
    return true;
}

} // anonymous namespace

// ---- Basic Equivalence Tests ----

TEST(CliParser, SimpleCommandEquivalence) {
    Parser parser;

    // Test: "help" command
    auto argv_cmd = parse_as_argv(parser, {"help"});
    auto repl_cmd = parser.parse_string("help");

    ASSERT_TRUE(commands_equal(argv_cmd, repl_cmd));
    ASSERT_EQ(argv_cmd.name, "help");
    ASSERT_TRUE(argv_cmd.positionals.empty());
    ASSERT_TRUE(argv_cmd.options.empty());
    ASSERT_TRUE(argv_cmd.flags.empty());
}

TEST(CliParser, CommandWithPositionalEquivalence) {
    Parser parser;

    // Test: "load import.db"
    auto argv_cmd = parse_as_argv(parser, {"load", "import.db"});
    auto repl_cmd = parser.parse_string("load import.db");

    ASSERT_TRUE(commands_equal(argv_cmd, repl_cmd));
    ASSERT_EQ(argv_cmd.name, "load");
    ASSERT_EQ(argv_cmd.positionals.size(), 1u);
    ASSERT_EQ(argv_cmd.positionals[0], "import.db");
}

TEST(CliParser, CommandWithFlagEquivalence) {
    Parser parser;

    // Test: "load import.db --verbose"
    auto argv_cmd = parse_as_argv(parser, {"load", "import.db", "--verbose"});
    auto repl_cmd = parser.parse_string("load import.db --verbose");

    ASSERT_TRUE(commands_equal(argv_cmd, repl_cmd));
    ASSERT_EQ(argv_cmd.name, "load");
    ASSERT_EQ(argv_cmd.positionals.size(), 1u);
    ASSERT_EQ(argv_cmd.positionals[0], "import.db");
    ASSERT_EQ(argv_cmd.flags.count("verbose"), 1u);
}

TEST(CliParser, CommandWithShortFlagEquivalence) {
    Parser parser;

    // Test: "load import.db -v"
    auto argv_cmd = parse_as_argv(parser, {"load", "import.db", "-v"});
    auto repl_cmd = parser.parse_string("load import.db -v");

    ASSERT_TRUE(commands_equal(argv_cmd, repl_cmd));
    ASSERT_EQ(argv_cmd.flags.count("v"), 1u);
}

// ---- Option Value Tests ----

TEST(CliParser, OptionWithSpaceSeparatedValueEquivalence) {
    Parser parser;

    // Test: "format --output json"
    auto argv_cmd = parse_as_argv(parser, {"format", "--output", "json"});
    auto repl_cmd = parser.parse_string("format --output json");

    ASSERT_TRUE(commands_equal(argv_cmd, repl_cmd));
    ASSERT_EQ(argv_cmd.name, "format");
    ASSERT_EQ(argv_cmd.options.count("output"), 1u);
    ASSERT_EQ(argv_cmd.options.at("output"), "json");
}

TEST(CliParser, OptionWithEqualsSyntaxEquivalence) {
    Parser parser;

    // Test: "load data.db --format=csv"
    auto argv_cmd = parse_as_argv(parser, {"load", "data.db", "--format=csv"});
    auto repl_cmd = parser.parse_string("load data.db --format=csv");

    ASSERT_TRUE(commands_equal(argv_cmd, repl_cmd));
    ASSERT_EQ(argv_cmd.name, "load");
    ASSERT_EQ(argv_cmd.positionals.size(), 1u);
    ASSERT_EQ(argv_cmd.positionals[0], "data.db");
    ASSERT_EQ(argv_cmd.options.count("format"), 1u);
    ASSERT_EQ(argv_cmd.options.at("format"), "csv");
}

TEST(CliParser, MixedOptionsAndFlagsEquivalence) {
    Parser parser;

    // Test: "load data.db --format json --verbose -d"
    auto argv_cmd = parse_as_argv(parser, {"load", "data.db", "--format", "json", "--verbose", "-d"});
    auto repl_cmd = parser.parse_string("load data.db --format json --verbose -d");

    ASSERT_TRUE(commands_equal(argv_cmd, repl_cmd));
    ASSERT_EQ(argv_cmd.options.at("format"), "json");
    ASSERT_EQ(argv_cmd.flags.count("verbose"), 1u);
    ASSERT_EQ(argv_cmd.flags.count("d"), 1u);
}

// ---- Quoted String Tests (REPL-specific behavior) ----

TEST(CliParser, QuotedStringWithSpaces) {
    Parser parser;

    // In argv mode, the shell handles quotes, so we pass the unquoted value
    auto argv_cmd = parse_as_argv(parser, {"load", "my file.db"});

    // In REPL mode, our parser must handle quotes
    auto repl_cmd = parser.parse_string("load \"my file.db\"");

    ASSERT_TRUE(commands_equal(argv_cmd, repl_cmd));
    ASSERT_EQ(argv_cmd.positionals[0], "my file.db");
}

TEST(CliParser, SingleQuotedString) {
    Parser parser;

    auto argv_cmd = parse_as_argv(parser, {"load", "my file.db"});
    auto repl_cmd = parser.parse_string("load 'my file.db'");

    ASSERT_TRUE(commands_equal(argv_cmd, repl_cmd));
    ASSERT_EQ(repl_cmd.positionals[0], "my file.db");
}

TEST(CliParser, QuotedOptionValue) {
    Parser parser;

    // argv: shell removes quotes
    auto argv_cmd = parse_as_argv(parser, {"query", "--filter", "name = John"});

    // REPL: parser removes quotes
    auto repl_cmd = parser.parse_string("query --filter \"name = John\"");

    ASSERT_TRUE(commands_equal(argv_cmd, repl_cmd));
    ASSERT_EQ(argv_cmd.options.at("filter"), "name = John");
}

TEST(CliParser, QuotedOptionValueWithEquals) {
    Parser parser;

    auto argv_cmd = parse_as_argv(parser, {"query", "--filter=name = John"});
    auto repl_cmd = parser.parse_string("query --filter=\"name = John\"");

    ASSERT_TRUE(commands_equal(argv_cmd, repl_cmd));
    ASSERT_EQ(argv_cmd.options.at("filter"), "name = John");
}

TEST(CliParser, EscapedQuoteInDoubleQuotes) {
    Parser parser;

    // Test escaped quote inside double quotes
    auto argv_cmd = parse_as_argv(parser, {"echo", "say \"hello\""});
    auto repl_cmd = parser.parse_string("echo \"say \\\"hello\\\"\"");

    ASSERT_TRUE(commands_equal(argv_cmd, repl_cmd));
    ASSERT_EQ(repl_cmd.positionals[0], "say \"hello\"");
}

TEST(CliParser, MixedQuotesAndUnquoted) {
    Parser parser;

    auto argv_cmd = parse_as_argv(parser, {"cmd", "arg1", "two words", "arg3"});
    auto repl_cmd = parser.parse_string("cmd arg1 \"two words\" arg3");

    ASSERT_TRUE(commands_equal(argv_cmd, repl_cmd));
    ASSERT_EQ(argv_cmd.positionals.size(), 3u);
    ASSERT_EQ(argv_cmd.positionals[0], "arg1");
    ASSERT_EQ(argv_cmd.positionals[1], "two words");
    ASSERT_EQ(argv_cmd.positionals[2], "arg3");
}

// ---- Edge Cases ----

TEST(CliParser, EmptyInput) {
    Parser parser;

    auto repl_cmd = parser.parse_string("");

    ASSERT_TRUE(repl_cmd.name.empty());
    ASSERT_TRUE(repl_cmd.positionals.empty());
    ASSERT_TRUE(repl_cmd.options.empty());
    ASSERT_TRUE(repl_cmd.flags.empty());
}

TEST(CliParser, WhitespaceOnlyInput) {
    Parser parser;

    auto repl_cmd = parser.parse_string("   \t  ");

    ASSERT_TRUE(repl_cmd.name.empty());
}

TEST(CliParser, MultiplePositionals) {
    Parser parser;

    auto argv_cmd = parse_as_argv(parser, {"copy", "src.txt", "dst.txt", "backup.txt"});
    auto repl_cmd = parser.parse_string("copy src.txt dst.txt backup.txt");

    ASSERT_TRUE(commands_equal(argv_cmd, repl_cmd));
    ASSERT_EQ(argv_cmd.positionals.size(), 3u);
}

TEST(CliParser, OptionBeforePositional) {
    Parser parser;

    // In schema-less parsing, an option followed by a non-option is treated
    // as option-with-value, not flag-then-positional.
    // This is intentional: without a schema, we can't distinguish them.
    auto argv_cmd = parse_as_argv(parser, {"load", "--verbose", "data.db"});
    auto repl_cmd = parser.parse_string("load --verbose data.db");

    ASSERT_TRUE(commands_equal(argv_cmd, repl_cmd));
    // --verbose consumes "data.db" as its value
    ASSERT_EQ(argv_cmd.options.count("verbose"), 1u);
    ASSERT_EQ(argv_cmd.options.at("verbose"), "data.db");
    ASSERT_TRUE(argv_cmd.positionals.empty());
}

TEST(CliParser, FlagAtEnd) {
    Parser parser;

    // Flags work correctly when they appear at the end (no following value)
    auto argv_cmd = parse_as_argv(parser, {"load", "data.db", "--verbose"});
    auto repl_cmd = parser.parse_string("load data.db --verbose");

    ASSERT_TRUE(commands_equal(argv_cmd, repl_cmd));
    ASSERT_EQ(argv_cmd.flags.count("verbose"), 1u);
    ASSERT_EQ(argv_cmd.positionals.size(), 1u);
    ASSERT_EQ(argv_cmd.positionals[0], "data.db");
}

TEST(CliParser, EmptyQuotedString) {
    Parser parser;

    auto argv_cmd = parse_as_argv(parser, {"cmd", ""});
    auto repl_cmd = parser.parse_string("cmd \"\"");

    ASSERT_TRUE(commands_equal(argv_cmd, repl_cmd));
    ASSERT_EQ(argv_cmd.positionals.size(), 1u);
    ASSERT_EQ(argv_cmd.positionals[0], "");
}

TEST(CliParser, OptionWithEmptyValue) {
    Parser parser;

    // --name="" should result in options["name"] = ""
    auto argv_cmd = parse_as_argv(parser, {"cmd", "--name="});
    auto repl_cmd = parser.parse_string("cmd --name=");

    ASSERT_TRUE(commands_equal(argv_cmd, repl_cmd));
    ASSERT_EQ(argv_cmd.options.count("name"), 1u);
    ASSERT_EQ(argv_cmd.options.at("name"), "");
}

// ---- ADR-006 Invariant Test ----

TEST(CliParser, ADR006InvariantExample) {
    // The exact example from ADR-006:
    // gtaf load import.db --verbose == REPL> load import.db --verbose

    Parser parser;

    auto argv_cmd = parse_as_argv(parser, {"load", "import.db", "--verbose"});
    auto repl_cmd = parser.parse_string("load import.db --verbose");

    ASSERT_TRUE(commands_equal(argv_cmd, repl_cmd));
    ASSERT_EQ(argv_cmd.name, "load");
    ASSERT_EQ(argv_cmd.positionals.size(), 1u);
    ASSERT_EQ(argv_cmd.positionals[0], "import.db");
    ASSERT_EQ(argv_cmd.flags.count("verbose"), 1u);
}
