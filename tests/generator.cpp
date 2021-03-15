////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [pfs-parser](https://github.com/semenovf/pfs-parser) library.
//
// Changelog:
//      2021.02.02 Initial version
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "pfs/parser/core_rules.hpp"
#include "pfs/parser/generator.hpp"
#include <iterator>
#include <vector>

using forward_iterator = std::vector<char>::const_iterator;

TEST_CASE("advance_repetition_by_range") {
    using pfs::parser::is_alpha_char;
    using pfs::parser::advance_repetition_by_range;

    struct test_item {
        bool success;
        int distance;
        std::vector<char> data;
        std::pair<int, int> range;
    };

    std::vector<test_item> test_values = {
          { true , 1, {'a'}, {0, 1} }
        , { true , 2, {'a', 'b'}, {1, 2} }
        , { false, 0, {'9'}, {1, 0} }
    };

    for (auto const & item : test_values) {
        auto first = item.data.begin();
        auto last = item.data.end();
        auto pos = first;

        auto result = advance_repetition_by_range(pos, last, item.range
            , [] (forward_iterator & first, forward_iterator last) {
                if (is_alpha_char(*first)) {
                    ++first;
                    return true;
                }
                return false;
            });

        CHECK(result == item.success);
        CHECK(static_cast<int>(std::distance(first, pos)) == item.distance);
    }
}

