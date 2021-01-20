////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [pfs-parser](https://github.com/semenovf/pfs-parser) library.
//
// Changelog:
//      2021.01.18 Initial version
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "pfs/parser/abnf/parser.hpp"
#include <vector>

TEST_CASE("is_prose_value_char") {
    using pfs::parser::abnf::is_prose_value_char;

    CHECK(is_prose_value_char(' '));
    CHECK(is_prose_value_char('\x20'));
    CHECK(is_prose_value_char('\x3D'));
    CHECK(is_prose_value_char('\x3F'));
    CHECK(is_prose_value_char('\x7E'));

    CHECK(is_prose_value_char('a'));
    CHECK(is_prose_value_char('z'));
    CHECK(is_prose_value_char('A'));
    CHECK(is_prose_value_char('Z'));

    CHECK(is_prose_value_char('0'));
    CHECK(is_prose_value_char('9'));

    CHECK_FALSE(is_prose_value_char('\x19'));
    CHECK_FALSE(is_prose_value_char('\x3E'));
    CHECK_FALSE(is_prose_value_char('\x7F'));
}

TEST_CASE("advance_prose_value") {
    using pfs::parser::abnf::advance_prose_value;

    std::vector<char> valid_values[] = {
          {'<', '>'}
        , {'<', ' ', '>'}
        , {'<', '\x20', '>'}
        , {'<', '\x3D', '>'}
        , {'<', '\x3F', '>'}
        , {'<', '\x7E', '>'}
        , {'<', ' ', '\x20', ' ', '>'}
        , {'<', ' ', '\x3D', ' ', '>'}
        , {'<', ' ', '\x3F', ' ', '>'}
        , {'<', ' ', '\x7E', ' ', '>'}
        , {'<', ' ', 'x', ' ', '>'}
    };

    std::vector<char> invalid_values[] = {
          {' '}
        , {'<'}
        , {'>'}
        , {'<', '\x19', '>'}
        , {'<', '\x7F', '>'}
        , {'<', ' ', 'x', ' '}
    };

    for (auto const & item: valid_values) {
        auto pos = item.begin();
        CHECK(advance_prose_value(pos, item.end()));
        CHECK(pos == item.end());
    }

    for (auto const & item: invalid_values) {
        auto pos = item.begin();
        CHECK_FALSE(advance_prose_value(pos, item.end()));
    }
}

using forward_iterator = std::vector<char>::const_iterator;

struct number_value_callbacks
{
    void first_number_value (pfs::parser::abnf::number_flag, forward_iterator, forward_iterator) {}
    void last_number_value (pfs::parser::abnf::number_flag, forward_iterator, forward_iterator) {}
    void next_number_value (pfs::parser::abnf::number_flag, forward_iterator, forward_iterator) {}
};

TEST_CASE("advance_number_value") {
    using pfs::parser::abnf::advance_number_value;

    std::vector<char> valid_values[] = {
          {'%', 'b', '0'}
        , {'%', 'b', '1'}
        , {'%', 'b', '0', '1', '1'}                // %b001
        , {'%', 'b', '0', '-', '1'}                // %b0-1
        , {'%', 'b', '0', '0', '-', '1', '1'}      // %b00-11
        , {'%', 'b', '0', '.', '1', '.', '1', '1'} // %b0.1.11

        , {'%', 'd', '0'}
        , {'%', 'd', '9'}
        , {'%', 'd', '1', '2', '3'}                // %d123
        , {'%', 'd', '0', '-', '9'}                // %b0-9
        , {'%', 'd', '0', '0', '-', '9', '9'}      // %b00-99
        , {'%', 'd', '2', '.', '3', '.', '4', '5'} // %b2.3.45

        , {'%', 'x', '0'}
        , {'%', 'x', 'F'}
        , {'%', 'x', 'a', 'B', 'c'}                // %daBc
        , {'%', 'x', '0', '-', 'F'}                // %b0-F
        , {'%', 'x', '0', '0', '-', 'f', 'f'}      // %b00-ff
        , {'%', 'x', '9', '.', 'A', '.', 'b', 'C'} // %b9.A.bC
    };

    std::vector<char> invalid_values[] = {
          {'x'}
        , {'%'}
        , {'%', 'b'}
        , {'%', 'b', '2'}
        , {'%', 'b', '-'}
        , {'%', 'b', '.'}
        , {'%', 'b', '1', '-'}
        , {'%', 'b', '1', '.'}

        , {'%', 'd'}
        , {'%', 'd', 'a'}
        , {'%', 'd', '-'}
        , {'%', 'd', '.'}
        , {'%', 'd', '9', '-'}
        , {'%', 'd', '9', '.'}

        , {'%', 'x'}
        , {'%', 'x', 'z'}
        , {'%', 'x', '-'}
        , {'%', 'x', '.'}
        , {'%', 'x', 'F', '-'}
        , {'%', 'x', 'F', '.'}
    };

    number_value_callbacks * callbacks = nullptr;

//     {
//         std::vector<char> item = {'%', 'b'};
//         auto pos = item.begin();
//         advance_number_value(pos, item.end(), callbacks);
//     }

    for (auto const & item: valid_values) {
        auto pos = item.begin();
        CHECK(advance_number_value(pos, item.end(), callbacks));
        CHECK(pos == item.end());
    }

    for (auto const & item: invalid_values) {
        auto pos = item.begin();
        CHECK_FALSE(advance_number_value(pos, item.end(), callbacks));
    }
}
