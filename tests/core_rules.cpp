#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "pfs/parser/core_rules.hpp"
#include <string>
#include <vector>

TEST_CASE("is_alpha") {
    using pfs::parser::is_alpha;

    CHECK(is_alpha('a'));
    CHECK(is_alpha('z'));
    CHECK(is_alpha('A'));
    CHECK(is_alpha('Z'));
    CHECK_FALSE(is_alpha('0'));
    CHECK_FALSE(is_alpha('9'));
    CHECK_FALSE(is_alpha(' '));
    CHECK_FALSE(is_alpha('\t'));
    CHECK_FALSE(is_alpha('\r'));
    CHECK_FALSE(is_alpha('\n'));
}

TEST_CASE("is_bit") {
    using pfs::parser::is_bit;

    CHECK(is_bit('0'));
    CHECK(is_bit('1'));
    CHECK_FALSE(is_bit('2'));
    CHECK_FALSE(is_bit('A'));
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
