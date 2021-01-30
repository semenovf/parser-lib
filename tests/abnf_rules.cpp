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

using pfs::parser::error_code;

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

struct quoted_string_context
{
    size_t max_length = 0;
    std::string result_str;
    error_code result_ec = pfs::parser::make_error_code(pfs::parser::errc::success);

    quoted_string_context (size_t limit = 0)
    {
        max_length = limit;
    }

    void quoted_string (std::string::const_iterator first
        , std::string::const_iterator last)
    {
        result_str = std::string(first, last);
    }

    void error (error_code ec)
    {
        result_ec = ec;
    }

    size_t max_quoted_string_length ()
    {
        return max_length;
    }
};

struct dummy_quoted_string_context
{
    void quoted_string (forward_iterator, forward_iterator) {}
    void error (error_code) {}
    size_t max_quoted_string_length () { return 0; }
};

TEST_CASE("advance_quoted_string") {
    using pfs::parser::abnf::advance_quoted_string;

    std::vector<char> valid_values[] = {
          {'"', '"'}
        , {'"', ' ', '"'}
        , {'"', '\x21', ' ', '\x7E', '"'}
    };

    std::vector<char> invalid_values[] = {
          {'x'}
        , {'"'}
        , {'"', 'x'}
        , {'"', '\x19', '"'}
        , {'"', '\x7F', '"'}
    };

    {
        dummy_quoted_string_context * ctx = nullptr;

        for (auto const & item: valid_values) {
            auto pos = item.begin();
            CHECK(advance_quoted_string(pos, item.end(), ctx));
            CHECK(pos == item.end());
        }

        for (auto const & item: invalid_values) {
            auto pos = item.begin();
            CHECK_FALSE(advance_quoted_string(pos, item.end(), ctx));
        }
    }

    {
        quoted_string_context ctx;
        std::string s {"\"Hello, World!\""};
        auto pos = s.begin();
        CHECK(advance_quoted_string(pos, s.end(), & ctx));
        CHECK(pos == s.end());
        CHECK(ctx.result_str == std::string{"Hello, World!"});
        CHECK(ctx.result_ec == pfs::parser::make_error_code(pfs::parser::errc::success));
    }

    {
        quoted_string_context ctx;
        std::string s {"\"x"};
        auto pos = s.begin();
        CHECK_FALSE(advance_quoted_string(pos, s.end(), & ctx));
        CHECK(pos == s.begin());
        CHECK(ctx.result_ec == pfs::parser::make_error_code(pfs::parser::errc::unbalanced_quote));
    }

    {
        quoted_string_context ctx;
        std::string s {"\"\x01\""};
        auto pos = s.begin();
        CHECK_FALSE(advance_quoted_string(pos, s.end(), & ctx));
        CHECK(pos == s.begin());
        CHECK(ctx.result_ec == pfs::parser::make_error_code(pfs::parser::errc::bad_quoted_char));
    }

    {
        quoted_string_context ctx{2};
        std::string s {"\"xyz\""};
        auto pos = s.begin();
        CHECK_FALSE(advance_quoted_string(pos, s.end(), & ctx));
        CHECK(pos == s.begin());
        CHECK(ctx.result_ec == pfs::parser::make_error_code(pfs::parser::errc::max_lengh_exceeded));
    }
}
