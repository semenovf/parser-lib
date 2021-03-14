////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [pfs-parser](https://github.com/semenovf/pfs-parser) library.
//
// Changelog:
//      2021.02.14 Initial version
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define PFS_SYNTAX_TREE_TRACE_ENABLE 1
#include "doctest.h"
#include "pfs/parser/abnf/parser.hpp"
#include "pfs/parser/abnf/syntax_tree.hpp"
#include "utils/read_file.hpp"
#include <iterator>
#include <vector>
#include <cassert>

struct test_item
{
    std::string filename;
    int rulenames;
};

static std::vector<test_item> data_files {
      { "data/wsp.grammar", 1 }
    , { "data/prose.grammar", 1 }
    , { "data/comment.grammar", 1 }
    , { "data/abnf.grammar", 37 }
    , { "data/json-rfc4627.grammar", 30 }
    , { "data/json-rfc8259.grammar", 30 }
    , { "data/uri-rfc3986.grammar", 36 }
    , { "data/uri-geo-rfc58070.grammar", 27 }
};

using string_type = std::string;
using forward_iterator = pfs::parser::line_counter_iterator<string_type::const_iterator>;

TEST_CASE("Syntax Tree") {
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

        pfs::parser::abnf::syntax_tree_context<string_type::const_iterator, string_type> ctx;
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

        // TODO Uncomment after implementation complete
        // CHECK(ctx.rules_count() == item.rulenames);

        return true;
    });

    CHECK_MESSAGE(result, "Parse files tested successfuly");
}
