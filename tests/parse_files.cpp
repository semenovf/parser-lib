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
    , { "data/abnf.grammar", 37 }
    , { "data/json-rfc4627.grammar", 30 }
    , { "data/json-rfc8259.grammar", 30 }
};

using forward_iterator = pfs::parser::line_counter_iterator<std::string::const_iterator>;

std::string read_file (std::string const & filename)
{
    std::ifstream ifs(filename.c_str(), std::ios::binary | std::ios::ate);

    if (!ifs.is_open()) {
        std::cerr << "Open file " << filename << " failure\n";
        return std::string{};
    }

    std::noskipws(ifs);

    // See examples at https://en.cppreference.com/w/cpp/io/basic_istream/read
    // and http://www.cplusplus.com/reference/istream/istream/read/
    ifs.seekg(0, ifs.end);
    auto size = ifs.tellg();
    std::string str(size, '\0'); // construct string to stream size
    ifs.seekg(0);

    ifs.read(& str[0], size);

    if (ifs) {
        ; // success
    } else {
        str.clear(); // error
    }

    ifs.close();
    return str;
}

struct dummy_context
{
    int rulenames = 0;

    // ProseContext
    void prose (forward_iterator, forward_iterator) {}

    // NumberContext
    void first_number (pfs::parser::abnf::number_flag, forward_iterator, forward_iterator) {}
    void last_number (pfs::parser::abnf::number_flag, forward_iterator, forward_iterator) {}
    void next_number (pfs::parser::abnf::number_flag, forward_iterator, forward_iterator) {}

    // QuotedStringContext
    void quoted_string (forward_iterator, forward_iterator) {}
    void error (std::error_code ec)
    {
        std::cerr << "Parse error: " << ec.message() << std::endl;
    }
    size_t max_quoted_string_length () { return 0; }

    // RepeatContext
    void repeat (forward_iterator first_from, forward_iterator last_from
        , forward_iterator first_to, forward_iterator last_to) {}

    // CommentContext
    void comment (forward_iterator first, forward_iterator last) {}

    // RulenameContext
    void rulename (forward_iterator first, forward_iterator last) {}

    // RepetitionContext
    void begin_repetition () {}
    void end_repetition (bool success) {}

    // ConcatenationContext
    void begin_concatenation () {}
    void end_concatenation (bool success) {}

    // DefinedAsContext
    void accept_basic_rule_definition () { rulenames++; }
    void accept_incremental_alternatives () {}
};

TEST_CASE("Parse files") {
    using pfs::parser::abnf::advance_rulelist;

    bool result = std::all_of(data_files.cbegin()
            , data_files.cend()
            , [] (test_item const & item) {
        std::cout << "Parsing file: " << item.filename << std::endl;

        auto source = read_file(item.filename);

        if (source.empty()) {
            std::cerr << "ERROR: reading file failure or it is empty" << std::endl;
            return false;
        }

        dummy_context ctx;
        auto first = forward_iterator(source.begin());
        auto last  = forward_iterator(source.end());
        auto result = advance_rulelist(first, last, & ctx);

        if (!result || first != last) {
            std::cerr << "ERROR: advance_rulelist failed at " << first.lineno()
                << " line" << std::endl;
        }

        CHECK_MESSAGE(result, item.filename);
        CHECK((first == last));
        CHECK(ctx.rulenames == item.rulenames);

        return true;
    });

    CHECK_MESSAGE(result, "Parse files tested successfuly");
}

