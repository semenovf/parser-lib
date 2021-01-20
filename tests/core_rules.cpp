////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [pfs-parser](https://github.com/semenovf/pfs-parser) library.
//
// Changelog:
//      2021.01.17 Initial version
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "pfs/parser/core_rules.hpp"
#include <string>
#include <vector>

TEST_CASE("is_alpha_char") {
    using pfs::parser::is_alpha_char;

    CHECK(is_alpha_char('a'));
    CHECK(is_alpha_char('z'));
    CHECK(is_alpha_char('A'));
    CHECK(is_alpha_char('Z'));
    CHECK_FALSE(is_alpha_char('0'));
    CHECK_FALSE(is_alpha_char('9'));
    CHECK_FALSE(is_alpha_char(' '));
    CHECK_FALSE(is_alpha_char('\t'));
    CHECK_FALSE(is_alpha_char('\r'));
    CHECK_FALSE(is_alpha_char('\n'));
}

TEST_CASE("is_bit_char") {
    using pfs::parser::is_bit_char;

    CHECK(is_bit_char('0'));
    CHECK(is_bit_char('1'));
    CHECK_FALSE(is_bit_char('2'));
    CHECK_FALSE(is_bit_char('A'));
}

TEST_CASE("is_ascii_char") {
    using pfs::parser::is_ascii_char;

    CHECK(is_ascii_char(' '));
    CHECK(is_ascii_char('\t'));
    CHECK(is_ascii_char('\r'));
    CHECK(is_ascii_char('\n'));
    CHECK(is_ascii_char('x'));
    CHECK_FALSE(is_ascii_char('\x0'));
}

TEST_CASE("is_cr_char") {
    using pfs::parser::is_cr_char;

    CHECK(is_cr_char('\x0D'));
    CHECK(is_cr_char('\r'));
    CHECK_FALSE(is_cr_char(' '));
    CHECK_FALSE(is_cr_char('\t'));
    CHECK_FALSE(is_cr_char('\n'));
    CHECK_FALSE(is_cr_char('x'));
}

TEST_CASE("is_lf_char") {
    using pfs::parser::is_lf_char;

    CHECK(is_lf_char('\x0A'));
    CHECK(is_lf_char('\n'));
    CHECK_FALSE(is_lf_char(' '));
    CHECK_FALSE(is_lf_char('\t'));
    CHECK_FALSE(is_lf_char('\r'));
    CHECK_FALSE(is_lf_char('x'));
}

TEST_CASE("advance_newline") {
    using pfs::parser::advance_newline;

    {
        auto newline = {'\x0D', '\x0A'};
        auto pos = newline.begin();

        CHECK(advance_newline(pos, newline.end()));
        CHECK(pos == newline.end());
    }

    {
        auto newline = {'\x0D', '\x0A', 'x'};
        auto pos = newline.begin();
        auto endpos = newline.begin();
        ++ ++endpos;

        CHECK(advance_newline(pos, newline.end()));
        CHECK(pos == endpos);
    }

    {
        auto newline = {'\x0A', '\x0D' };
        auto pos = newline.begin();

        CHECK(advance_newline(pos, newline.end()));
    }

    {
        auto newline = {'x', '\x0A'};
        auto pos = newline.begin();
        CHECK_FALSE(advance_newline(pos, newline.end()));
    }
}

TEST_CASE("is_control_char") {
    using pfs::parser::is_control_char;

    CHECK(is_control_char('\x00'));
    CHECK(is_control_char('\x1F'));
    CHECK(is_control_char('\x7F'));
    CHECK(is_control_char('\x0A'));
    CHECK(is_control_char('\n'));
    CHECK(is_control_char('\t'));
    CHECK(is_control_char('\r'));
    CHECK_FALSE(is_control_char(' '));
    CHECK_FALSE(is_control_char('x'));
}

TEST_CASE("is_digit_char") {
    using pfs::parser::is_digit_char;

    CHECK(is_digit_char('0'));
    CHECK(is_digit_char('1'));
    CHECK(is_digit_char('2'));
    CHECK(is_digit_char('3'));
    CHECK(is_digit_char('4'));
    CHECK(is_digit_char('5'));
    CHECK(is_digit_char('6'));
    CHECK(is_digit_char('7'));
    CHECK(is_digit_char('8'));
    CHECK(is_digit_char('9'));

    CHECK_FALSE(is_digit_char('A'));
    CHECK_FALSE(is_digit_char('x'));
    CHECK_FALSE(is_digit_char('\x00'));
}

TEST_CASE("is_hexdigit_char") {
    using pfs::parser::is_hexdigit_char;

    CHECK(is_hexdigit_char('0'));
    CHECK(is_hexdigit_char('1'));
    CHECK(is_hexdigit_char('2'));
    CHECK(is_hexdigit_char('3'));
    CHECK(is_hexdigit_char('4'));
    CHECK(is_hexdigit_char('5'));
    CHECK(is_hexdigit_char('6'));
    CHECK(is_hexdigit_char('7'));
    CHECK(is_hexdigit_char('8'));
    CHECK(is_hexdigit_char('9'));
    CHECK(is_hexdigit_char('A'));
    CHECK(is_hexdigit_char('F'));
    CHECK(is_hexdigit_char('a'));
    CHECK(is_hexdigit_char('f'));

    CHECK_FALSE(is_hexdigit_char('g'));
    CHECK_FALSE(is_hexdigit_char('G'));
    CHECK_FALSE(is_hexdigit_char('\x00'));
}

TEST_CASE("is_dquote_char") {
    using pfs::parser::is_dquote_char;

    CHECK(is_dquote_char('"'));
    CHECK(is_dquote_char('\x22'));
    CHECK_FALSE(is_dquote_char('\''));
}

TEST_CASE("is_htab_char") {
    using pfs::parser::is_htab_char;

    CHECK(is_htab_char('\t'));
    CHECK(is_htab_char('\x09'));
    CHECK_FALSE(is_htab_char('x'));
}

TEST_CASE("is_octet_char") {
    using pfs::parser::is_octet_char;

    CHECK(is_octet_char('\x00'));
    CHECK(is_octet_char('\xFF'));
    CHECK(is_octet_char('A'));
    CHECK(is_octet_char('Z'));
}

TEST_CASE("is_space_char") {
    using pfs::parser::is_space_char;

    CHECK(is_space_char(' '));
    CHECK_FALSE(is_space_char('\t'));
    CHECK_FALSE(is_space_char('\r'));
    CHECK_FALSE(is_space_char('\n'));
    CHECK_FALSE(is_space_char('x'));
}

TEST_CASE("is_visible_char") {
    using pfs::parser::is_visible_char;

    CHECK(is_visible_char('a'));
    CHECK(is_visible_char('z'));
    CHECK(is_visible_char('A'));
    CHECK(is_visible_char('Z'));
    CHECK(is_visible_char('0'));
    CHECK(is_visible_char('9'));
    CHECK_FALSE(is_visible_char(' '));
    CHECK_FALSE(is_visible_char('\t'));
    CHECK_FALSE(is_visible_char('\r'));
    CHECK_FALSE(is_visible_char('\n'));
}

TEST_CASE("advance_linear_whitespace") {
    using pfs::parser::advance_linear_whitespace;

    std::vector<char> linear_whitespaces[] = {
          {' '}
        , {'\n'}
        , {'\r'}
        , {' ', ' '}
        , {' ', '\n'}
        , {' ', '\r'}
        , {' ', '\n', '\r'}
        , {' ', '\r', '\n'}
        , {' ', '\n', '\r', ' '}
        , {' ', '\r', '\n', ' '}
    };

    for (auto const & item: linear_whitespaces) {
        auto pos = item.begin();
        CHECK(advance_linear_whitespace(pos, item.end()));
        CHECK(pos == item.end());
    }

    {
        auto linear_whitespace = {'x'};
        auto pos = linear_whitespace.begin();

        CHECK_FALSE(advance_linear_whitespace(pos, linear_whitespace.end()));
        CHECK(pos == linear_whitespace.begin());
    }
}

TEST_CASE("advance_bit_chars") {
    using pfs::parser::advance_bit_chars;

    std::vector<char> bit_chars[] = {
          {'1'}
        , {'1', '1'}
        , {'0'}
        , {'0', '0'}
        , {'1', '0'}
        , {'1', '0', '1'}
    };

    std::vector<char> bad_bit_chars[] = {
          {'x'}
    };

    for (auto const & item: bit_chars) {
        auto pos = item.begin();
        CHECK(advance_bit_chars(pos, item.end()));
        CHECK(pos == item.end());
    }

    for (auto const & item: bad_bit_chars) {
        auto pos = item.begin();

        CHECK_FALSE(advance_bit_chars(pos, item.end()));
        CHECK(pos == item.begin());
    }
}

TEST_CASE("advance_digit_chars") {
    using pfs::parser::advance_digit_chars;

    std::vector<char> digit_chars[] = {
          {'1'}
        , {'1', '1'}
        , {'0'}
        , {'0', '0'}
        , {'1', '0'}
        , {'1', '0', '1'}
        , {'9', '8', '7'}
    };

    std::vector<char> bad_digit_chars[] = {
          {'x'}
    };

    for (auto const & item: digit_chars) {
        auto pos = item.begin();
        CHECK(advance_digit_chars(pos, item.end()));
        CHECK(pos == item.end());
    }

    for (auto const & item: bad_digit_chars) {
        auto pos = item.begin();

        CHECK_FALSE(advance_digit_chars(pos, item.end()));
        CHECK(pos == item.begin());
    }
}

TEST_CASE("advance_hexdigit_chars") {
    using pfs::parser::advance_hexdigit_chars;

    std::vector<char> hexdigit_chars[] = {
          {'1'}
        , {'1', '1'}
        , {'0'}
        , {'0', '0'}
        , {'1', '0'}
        , {'1', '0', '1'}
        , {'9', '8', '7'}
        , {'A', 'b', 'c'}
    };

    std::vector<char> bad_hexdigit_chars[] = {
          {'x'}
    };

    for (auto const & item: hexdigit_chars) {
        auto pos = item.begin();
        CHECK(advance_hexdigit_chars(pos, item.end()));
        CHECK(pos == item.end());
    }

    for (auto const & item: bad_hexdigit_chars) {
        auto pos = item.begin();

        CHECK_FALSE(advance_hexdigit_chars(pos, item.end()));
        CHECK(pos == item.begin());
    }
}
