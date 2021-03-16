////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [pfs-parser](https://github.com/semenovf/pfs-parser) library.
//
// Changelog:
//      2021.02.07 Initial version
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "pfs/parser/abnf/parser.hpp"
#include "pfs/parser/line_counter_iterator.hpp"
#include "utils/read_file.hpp"
#include <iterator>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>

struct test_item
{
    std::string filename;
    int rulenames;
};

static std::vector<test_item> data_files {
      { "data/wsp.grammar", 1 }
    , { "data/prose.grammar", 1 }
    , { "data/abnf.grammar", 37 }
    , { "data/json-rfc4627.grammar", 30 }
    , { "data/json-rfc8259.grammar", 30 }
    , { "data/uri-rfc3986.grammar", 36 }
    , { "data/uri-geo-rfc58070.grammar", 27 }
};

using forward_iterator = pfs::parser::line_counter_iterator<std::string::const_iterator>;

struct dummy_context
{
    int rulenames = 0;

    // ProseContext
    void prose (forward_iterator, forward_iterator) {}

    // NumberContext
    void first_number (pfs::parser::abnf::number_flag, forward_iterator, forward_iterator) {}
    void last_number (pfs::parser::abnf::number_flag, forward_iterator, forward_iterator) {}
    void next_number (pfs::parser::abnf::number_flag, forward_iterator, forward_iterator) {} // LCOV_EXCL_LINE

    // QuotedStringContext
    void quoted_string (forward_iterator, forward_iterator) {}

    // LCOV_EXCL_START
    void error (std::error_code ec)
    {
        std::cerr << "Parse error: " << ec.message() << std::endl;
    }
    // LCOV_EXCL_STOP

    size_t max_quoted_string_length () { return 0; }

    // GroupContext
    void begin_group () {}
    void end_group (bool success) {}

    // OptionContext
    void begin_option () {}
    void end_option (bool success) {}

    // RepeatContext
    void repeat (long from, long to) {}

    // CommentContext
    void comment (forward_iterator first, forward_iterator last) {}

    // RulenameContext
    void rulename (forward_iterator first, forward_iterator last) {}

    // RepetitionContext
    void begin_repetition () {}
    void end_repetition (bool success) {}

    // AlternationContext
    void begin_alternation () {}
    void end_alternation (bool success) {}

    // ConcatenationContext
    void begin_concatenation () {}
    void end_concatenation (bool success) {}

    // RuleContext
    void begin_rule (forward_iterator, forward_iterator
        , bool is_incremental_alternatives)
    {
        if (!is_incremental_alternatives)
            rulenames++;
    }

    void end_rule (forward_iterator, forward_iterator, bool, bool) {}
};

TEST_CASE("Parse files") {
    using pfs::parser::abnf::advance_rulelist;

    bool result = std::all_of(data_files.cbegin()
            , data_files.cend()
            , [] (test_item const & item) {
        std::cout << "Parsing file: " << item.filename << std::endl;

        auto source = utils::read_file(item.filename);

        if (source.empty()) {
            // LCOV_EXCL_START
            std::cerr << "ERROR: reading file failure or it is empty" << std::endl;
            return false;
            // LCOV_EXCL_STOP
        }

        dummy_context ctx;
        auto first = forward_iterator(source.begin());
        auto last  = forward_iterator(source.end());
        auto result = advance_rulelist(first, last, & ctx);

        if (!result || first != last) {
            // LCOV_EXCL_START
            std::cerr << "ERROR: advance_rulelist failed at " << first.lineno()
                << " line" << std::endl;
            // LCOV_EXCL_STOP
        }

        CHECK_MESSAGE(result, item.filename);
        CHECK((first == last));
        CHECK(ctx.rulenames == item.rulenames);

        return true;
    });

    CHECK_MESSAGE(result, "Parse files tested successfuly");
}

