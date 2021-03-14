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
using forward_iterator = std::vector<char>::const_iterator;

struct test_item {
    bool success;
    int distance;
    std::vector<char> data;
};

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

struct dummy_prose_context
{
    // LCOV_EXCL_START
    void prose (forward_iterator, forward_iterator) {}
    // LCOV_EXCL_STOP
};

TEST_CASE("advance_prose") {
    using pfs::parser::abnf::advance_prose;

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

    dummy_prose_context * ctx = nullptr;

    for (auto const & item: valid_values) {
        auto pos = item.begin();
        CHECK(advance_prose(pos, item.end(), ctx));
        CHECK(pos == item.end());
    }

    for (auto const & item: invalid_values) {
        auto pos = item.begin();
        CHECK_FALSE(advance_prose(pos, item.end(), ctx));
    }
}

struct dummy_number_context
{
    // LCOV_EXCL_START
    void first_number (pfs::parser::abnf::number_flag, forward_iterator, forward_iterator) {}
    void last_number (pfs::parser::abnf::number_flag, forward_iterator, forward_iterator) {}
    void next_number (pfs::parser::abnf::number_flag, forward_iterator, forward_iterator) {}
    // LCOV_EXCL_STOP
};

TEST_CASE("advance_number") {
    using pfs::parser::abnf::advance_number;

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

    dummy_number_context * ctx = nullptr;

//     {
//         std::vector<char> item = {'%', 'b'};
//         auto pos = item.begin();
//         advance_number(pos, item.end(), callbacks);
//     }

    for (auto const & item: valid_values) {
        auto pos = item.begin();
        CHECK(advance_number(pos, item.end(), ctx));
        CHECK(pos == item.end());
    }

    for (auto const & item: invalid_values) {
        auto pos = item.begin();
        CHECK_FALSE(advance_number(pos, item.end(), ctx));
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
    // LCOV_EXCL_START
    void quoted_string (forward_iterator, forward_iterator) {}
    void error (error_code) {}
    size_t max_quoted_string_length () { return 0; }
    // LCOV_EXCL_STOP
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
        CHECK(ctx.result_ec == pfs::parser::make_error_code(pfs::parser::errc::max_length_exceeded));
    }
}

struct dummy_repeat_context
{
    // LCOV_EXCL_START
    void repeat (long from, long to) {}
    void error (error_code ec) {}
    // LCOV_EXCL_STOP
};

struct repeat_context
{
    using forward_iterator = std::string::const_iterator;

    long from;
    long to;

    void repeat (long f, long t)
    {
        from = f;
        to = t;
    }

    void error (error_code ec) {}
};

TEST_CASE("advance_repeat") {
    using pfs::parser::abnf::advance_repeat;

    std::vector<char> valid_values[] = {
          {'1'}
        , {'1', '2'}
        , {'*'}
        , {'*', '2'}
        , {'1', '*'}
        , {'1', '*', '2'}
        , {'1', '0', '*', '2', '0'}
    };

    std::vector<char> invalid_values[] = {
          {'x'}
        , {'x', '*'}
    };

    {
        dummy_repeat_context * ctx = nullptr;

        for (auto const & item: valid_values) {
            auto pos = item.begin();
            CHECK(advance_repeat(pos, item.end(), ctx));
            CHECK(pos == item.end());
        }

        for (auto const & item: invalid_values) {
            auto pos = item.begin();
            CHECK_FALSE(advance_repeat(pos, item.end(), ctx));
        }
    }

    {
        repeat_context ctx;
        std::string s {"10"};
        auto pos = s.begin();
        CHECK(advance_repeat(pos, s.end(), & ctx));
        CHECK(pos == s.end());
        CHECK(ctx.from == 10L);
        CHECK(ctx.to == 10L);
    }

    {
        repeat_context ctx;
        std::string s {"10*"};
        auto pos = s.begin();
        CHECK(advance_repeat(pos, s.end(), & ctx));
        CHECK(pos == s.end());
        CHECK(ctx.from == 10L);
        CHECK(ctx.to == std::numeric_limits<long>::max());
    }

    {
        repeat_context ctx;
        std::string s {"*10"};
        auto pos = s.begin();
        CHECK(advance_repeat(pos, s.end(), & ctx));
        CHECK(pos == s.end());
        CHECK(ctx.from == 0);
        CHECK(ctx.to == 10L);
    }

    {
        repeat_context ctx;
        std::string s {"*"};
        auto pos = s.begin();
        CHECK(advance_repeat(pos, s.end(), & ctx));
        CHECK(pos == s.end());
        CHECK(ctx.from == 0);
        CHECK(ctx.to == std::numeric_limits<long>::max());
    }

    {
        repeat_context ctx;
        std::string s {"10*20"};
        auto pos = s.begin();
        CHECK(advance_repeat(pos, s.end(), & ctx));
        CHECK(pos == s.end());
        CHECK(ctx.from == 10L);
        CHECK(ctx.to == 20L);
    }

    {
        repeat_context ctx;
        std::string s {"*x"};
        auto pos = s.begin();
        CHECK(advance_repeat(pos, s.end(), & ctx));
        CHECK_FALSE(pos == s.end());
        CHECK(ctx.from == 0);
        CHECK(ctx.to == std::numeric_limits<long>::max());
    }
}

TEST_CASE("advance_comment") {
    using pfs::parser::abnf::advance_comment;

    std::vector<char> valid_values[] = {
          {';'}
        , {';', '\n'}
        , {';', '\r'}
        , {';', '\r', '\n'}
        , {';', 'c', '\r', '\n' }
        , {';', ' ', 'c', 'o', 'm', 'm', 'e', 'n', 't', '\t', '\n'}
    };

    std::vector<char> invalid_values[] = {
        {'x'}
    };

    {
        for (auto const & item: valid_values) {
            auto pos = item.begin();
            CHECK(advance_comment(pos, item.end()));
            CHECK(pos == item.end());
        }

        for (auto const & item: invalid_values) {
            auto pos = item.begin();
            CHECK_FALSE(advance_comment(pos, item.end()));
        }
    }

    {
        std::string s {";"};
        auto pos = s.begin();
        CHECK(advance_comment(pos, s.end()));
        CHECK(pos == s.end());
    }

    {
        std::string s {";\r\n"};
        auto pos = s.begin();
        CHECK(advance_comment(pos, s.end()));
        CHECK(pos == s.end());
    }

    {
        std::string s {"; comment "};
        auto pos = s.begin();
        CHECK(advance_comment(pos, s.end()));
        CHECK(pos == s.end());
    }

    {
        std::string s {"; comment \r\n"};
        auto pos = s.begin();
        CHECK(advance_comment(pos, s.end()));
        CHECK(pos == s.end());
    }
}

TEST_CASE("advance_comment_newline") {
    using pfs::parser::abnf::advance_comment_newline;

    std::vector<char> valid_values[] = {
          {';'}
        , {';', '\n'}
        , {';', '\r'}
        , {';', '\r', '\n'}
        , {';', 'c', '\r', '\n' }
        , {';', ' ', 'c', 'o', 'm', 'm', 'e', 'n', 't', '\t', '\n'}
        , {'\r'}
        , {'\r', '\n'}
        , {'\n'}
    };

    std::vector<char> invalid_values[] = {
        {'x'}
    };

    {
        for (auto const & item: valid_values) {
            auto pos = item.begin();
            CHECK(advance_comment_newline(pos, item.end()));
            CHECK(pos == item.end());
        }

        for (auto const & item: invalid_values) {
            auto pos = item.begin();
            CHECK_FALSE(advance_comment_newline(pos, item.end()));
        }
    }
}

TEST_CASE("advance_comment_whitespace") {
    using pfs::parser::abnf::advance_comment_whitespace;

    std::vector<char> valid_values[] = {
          {' '}
        , {'\t'}
        , {';', '\n', ' '}
        , {';', '\n', '\t'}
        , {';', '\r', ' '}
        , {';', '\r', '\t'}
        , {';', '\r', '\n', ' '}
        , {';', 'c', '\r', '\n', '\t' }
        , {';', ' ', 'c', 'o', 'm', 'm', 'e', 'n', 't', '\t', '\n', ' '}
    };

    std::vector<char> invalid_values[] = {
          {'x'}
        , {';'}
        , {';', '\n'}
    };

    {
        for (auto const & item: valid_values) {
            auto pos = item.begin();
            CHECK(advance_comment_whitespace(pos, item.end()));
            CHECK(pos == item.end());
        }

        for (auto const & item: invalid_values) {
            auto pos = item.begin();
            CHECK_FALSE(advance_comment_whitespace(pos, item.end()));
        }
    }
}

struct dummy_rulename_context
{
    // LCOV_EXCL_START
    void rulename (forward_iterator first, forward_iterator last) {}
    // LCOV_EXCL_STOP
};

struct rulename_context
{
    using forward_iterator = std::string::const_iterator;

    std::string name;

    void rulename (forward_iterator first, forward_iterator last)
    {
        name = std::string{first, last};
    }
};

TEST_CASE("advance_rulename") {
    using pfs::parser::abnf::advance_rulename;

    std::vector<char> valid_values[] = {
          {'A'}
        , {'A', '1'}
        , {'A', '1', '-'}
        , {'A', '-', '1'}
    };

    std::vector<char> invalid_values[] = {
          {'1'}
        , {'-'}
        , {' '}
    };

    {
        dummy_rulename_context * ctx = nullptr;

        for (auto const & item: valid_values) {
            auto pos = item.begin();
            CHECK(advance_rulename(pos, item.end(), ctx));
            CHECK(pos == item.end());
        }

        for (auto const & item: invalid_values) {
            auto pos = item.begin();
            CHECK_FALSE(advance_rulename(pos, item.end(), ctx));
        }
    }

//     {
//         repeat_context ctx;
//         std::string s {"10"};
//         auto pos = s.begin();
//         CHECK(advance_repeat(pos, s.end(), & ctx));
//         CHECK(pos == s.end());
//         CHECK(ctx.from == std::string{"10"});
//         CHECK(ctx.to == ctx.from);
//     }
//
//     {
//         repeat_context ctx;
//         std::string s {"10*"};
//         auto pos = s.begin();
//         CHECK(advance_repeat(pos, s.end(), & ctx));
//         CHECK(pos == s.end());
//         CHECK(ctx.from == std::string{"10"});
//         CHECK(ctx.to.empty());
//     }
//
//     {
//         repeat_context ctx;
//         std::string s {"*10"};
//         auto pos = s.begin();
//         CHECK(advance_repeat(pos, s.end(), & ctx));
//         CHECK(pos == s.end());
//         CHECK(ctx.from.empty());
//         CHECK(ctx.to == std::string{"10"});
//     }
//
//     {
//         repeat_context ctx;
//         std::string s {"*"};
//         auto pos = s.begin();
//         CHECK(advance_repeat(pos, s.end(), & ctx));
//         CHECK(pos == s.end());
//         CHECK(ctx.from.empty());
//         CHECK(ctx.to.empty());
//     }
//
//     {
//         repeat_context ctx;
//         std::string s {"10*20"};
//         auto pos = s.begin();
//         CHECK(advance_repeat(pos, s.end(), & ctx));
//         CHECK(pos == s.end());
//         CHECK(ctx.from == std::string{"10"});
//         CHECK(ctx.to == std::string{"20"});
//     }
//
//     {
//         repeat_context ctx;
//         std::string s {"*x"};
//         auto pos = s.begin();
//         CHECK(advance_repeat(pos, s.end(), & ctx));
//         CHECK_FALSE(pos == s.end());
//         CHECK(ctx.from.empty());
//         CHECK(ctx.to.empty());
//     }
}

struct dummy_element_context
    : virtual public dummy_rulename_context
    , virtual public dummy_number_context
    , virtual public dummy_quoted_string_context
    , virtual public dummy_prose_context
{
    void repeat (long from, long to) {}

    // Below are requirements for GroupContext and OptionContext

    // GroupContext
    void begin_group () {}
    void end_group (bool success) {}

    // OptionContext
    void begin_option () {}
    void end_option (bool success) {}

    // LCOV_EXCL_START
    // ConcatenationContext
    void begin_concatenation () {}
    void end_concatenation (bool ) {}

    // RepetitionContext
    void begin_repetition () {}
    void end_repetition (bool) {}

    // AlternationContext
    void begin_alternation () {}
    void end_alternation (bool success) {}

    // LCOV_EXCL_STOP
};

TEST_CASE("advance_element") {
    using pfs::parser::abnf::advance_element;

    std::vector<char> valid_values[] = {
        // Prose
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

        // Number
        , {'%', 'b', '0'}
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

        // Quoted string
        , {'"', '"'}
        , {'"', ' ', '"'}
        , {'"', '\x21', ' ', '\x7E', '"'}

        // Rulename
        , {'A'}
        , {'A', '1'}
        , {'A', '1', '-'}
        , {'A', '-', '1'}
    };

    std::vector<char> invalid_values[] = {
        // Not a Prose
          {'<'}
        , {'>'}
        , {'<', '\x19', '>'}
        , {'<', '\x7F', '>'}
        , {'<', ' ', 'x', ' '}

        // Not a Number
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

        // Not a Quoted string
        , {'"'}
        , {'"', 'x'}
        , {'"', '\x19', '"'}
        , {'"', '\x7F', '"'}

        // Not a rulename
        , {'1'}
        , {'-'}
        , {' '}
    };

    {
        dummy_element_context * ctx = nullptr;

        for (auto const & item : valid_values) {
            auto pos = item.begin();
            CHECK(advance_element(pos, item.end(), ctx));
            CHECK(pos == item.end());
        }

        for (auto const & item : invalid_values) {
            auto pos = item.begin();
            CHECK_FALSE(advance_element(pos, item.end(), ctx));
        }
    }
}

struct dummy_repetition_context
    : dummy_element_context
{
    // LCOV_EXCL_START
    void begin_repetition () {}
    void end_repetition (bool) {}
    // LCOV_EXCL_STOP
};

TEST_CASE("advance_repetition") {
    using pfs::parser::abnf::advance_repetition;

    std::vector<char> valid_values[] = {
        // Prose
          {'<', '>'}
        , {'*', '<', '>'}
        , {'1', '*', '<', ' ', '>'}
        , {'*', '2', '<', '\x20', '>'}
        , {'1', '*', '2', '<', ' ', '\x3D', ' ', '>'}

        // Number
        , {'%', 'b', '0', '1', '1'}                     // %b001
        , {'*', '%', 'b', '0', '1', '1'}                // %b001
        , {'1', '*', '%', 'b', '0', '-', '1'}           // %b0-1
        , {'*', '2', '%', 'b', '0', '0', '-', '1', '1'} // %b00-11
        , {'1', '*', '2', '%', 'b', '0', '.', '1', '.', '1', '1'} // %b0.1.11

        // Quoted string
        , {'"', '"'}
        , {'*', '"', '"'}
        , {'1', '*', '"', ' ', '"'}
        , {'*', '2', '"', '\x21', ' ', '\x7E', '"'}
        , {'1', '*', '2', '"', '\x21', ' ', '\x7E', '"'}

        // Rulename
        , {'A'}
        , {'*', 'A'}
        , {'1', '*', 'A', '1'}
        , {'*', '2', 'A', '1', '-'}
        , {'1', '*', '2', 'A', '-', '1'}
    };

    std::vector<char> invalid_values[] = {
          {' '}
    };

    {
        dummy_repetition_context * ctx = nullptr;

        for (auto const & item : valid_values) {
            auto pos = item.begin();
            CHECK(advance_repetition(pos, item.end(), ctx));
            CHECK(pos == item.end());
        }

        for (auto const & item : invalid_values) {
            auto pos = item.begin();
            CHECK_FALSE(advance_repetition(pos, item.end(), ctx));
        }
    }
}

struct dummy_concatenation_context
    : dummy_repetition_context
{
    // LCOV_EXCL_START
    void begin_concatenation () {}
    void end_concatenation (bool ) {}
    // LCOV_EXCL_STOP
};

TEST_CASE("advance_concatenation") {
    using pfs::parser::abnf::advance_concatenation;

    std::vector<test_item> test_values = {
          { true, 1, {'a'} }
        , { true, 1, {'a', ' '} }
        , { true, 3, {'a', ' ', 'b'} }
        , { true, 4, {'a', ' ', '\t', 'b'} }
    };

    dummy_concatenation_context * ctx = nullptr;

    for (auto const & item : test_values) {
        auto first = item.data.begin();
        auto last = item.data.end();
        auto pos = first;

        auto result = advance_concatenation(pos, last, ctx);

        CHECK(result == item.success);
        CHECK(static_cast<int>(std::distance(first, pos)) == item.distance);
    }
}

struct dummy_alternation_context
    : dummy_concatenation_context
{
    void begin_alternation () {}
    void end_alternation (bool success) {}
};

TEST_CASE("advance_alternation") {
    using pfs::parser::abnf::advance_alternation;

    std::vector<test_item> test_values = {
          { true, 1, {'a'} }
        , { true, 1, {'a', ' '} }
        , { true, 3, {'a', ' ', 'b'} }
        , { true, 4, {'a', ' ', '\t', 'b'} }
        , { true, 3, {'a', '/', 'b'} }
        , { true, 4, {'a', '/', '\t', 'b'} }
        , { true, 5, {'a', ' ', '/', '\t', 'b'} }
    };

    dummy_alternation_context * ctx = nullptr;

    for (auto const & item : test_values) {
        auto first = item.data.begin();
        auto last = item.data.end();
        auto pos = first;

        auto result = advance_alternation(pos, last, ctx);

        CHECK(result == item.success);
        CHECK(static_cast<int>(std::distance(first, pos)) == item.distance);
    }
}

struct dummy_group_context
    : dummy_alternation_context
{
    // GroupContext
    void begin_group () {}
    void end_group (bool success) {}
};

TEST_CASE("advance_group") {
    using pfs::parser::abnf::advance_group;

    std::vector<test_item> test_values = {
          { false, 0, {'(', ')'} }
        , { true , 3, {'(', 'a', ')'} }
    };

    dummy_group_context * ctx = nullptr;

    for (auto const & item : test_values) {
        auto first = item.data.begin();
        auto last = item.data.end();
        auto pos = first;

        auto result = advance_group(pos, last, ctx);

        CHECK(result == item.success);
        CHECK(static_cast<int>(std::distance(first, pos)) == item.distance);
    }
}

struct dummy_option_context
    : dummy_alternation_context
{
    // OptionContext
    void begin_option () {}
    void end_option (bool success) {}
};

TEST_CASE("advance_group") {
    using pfs::parser::abnf::advance_option;

    std::vector<test_item> test_values = {
          { false, 0, {'[', ']'} }
        , { true , 3, {'[', 'a', ']'} }
    };

    dummy_option_context * ctx = nullptr;

    for (auto const & item : test_values) {
        auto first = item.data.begin();
        auto last = item.data.end();
        auto pos = first;

        auto result = advance_option(pos, last, ctx);

        CHECK(result == item.success);
        CHECK(static_cast<int>(std::distance(first, pos)) == item.distance);
    }
}

struct dummy_defined_as_context
{
    // LCOV_EXCL_START
    void accept_basic_rule_definition () {}
    void accept_incremental_alternatives () {}
    // LCOV_EXCL_STOP
};

TEST_CASE("advance_defined_as") {
    using pfs::parser::abnf::advance_defined_as;

    std::vector<test_item> test_values = {
          { true, 1, {'='} }
        , { true, 2, {'=', '/'} }
        , { true, 3, {' ', '=', '/'} }
        , { true, 4, {' ', '=', '/', '\t'} }
        , { true, 8, {';', '\n', '\t', '=', '/', ';', '\n', ' '} }
    };

    dummy_defined_as_context * ctx = nullptr;

    for (auto const & item : test_values) {
        auto first = item.data.begin();
        auto last = item.data.end();
        auto pos = first;

        auto result = advance_defined_as(pos, last, ctx);

        CHECK(result == item.success);
        CHECK(static_cast<int>(std::distance(first, pos)) == item.distance);
    }
}

struct dummy_elements_context
    : dummy_alternation_context
{};

TEST_CASE("advance_elements") {
    using pfs::parser::abnf::advance_elements;

    std::vector<test_item> test_values = {
          { true, 2, {'a', ' '} }
        , { true, 4, {'a', ';', '\n', '\t'} }
        , { true, 26, {'1', '*', '(', ' ', 'r', 'u', 'l', 'e', ' ', '/', ' '
                , '(', '*', 'c', '-', 'w', 's', 'p', ' ', 'c', '-', 'n', 'l'
                , ')', ' ', ')'} }
    };

    dummy_elements_context * ctx = nullptr;

    for (auto const & item : test_values) {
        auto first = item.data.begin();
        auto last = item.data.end();
        auto pos = first;

        auto result = advance_elements(pos, last, ctx);

        CHECK(result == item.success);
        CHECK(static_cast<int>(std::distance(first, pos)) == item.distance);
    }
}

struct dummy_rule_context
    : dummy_alternation_context
    , dummy_defined_as_context
{
    // RuleContext
    void begin_rule () {}
    void end_rule (bool success) {}
};

TEST_CASE("advance_rule") {
    using pfs::parser::abnf::advance_rule;

    std::vector<test_item> test_values = {
          { true, 9, {'r', ' ', '=', ' ', '[', 'p', ']', ' ', 'e'} }

        , { true, 34, {'r', 'e', 'p', 'e', 't', 'i', 't', 'i', 'o', 'n'
                , ' ', ' ', ' ', ' ', ' ', '=', ' ', ' ', '[', 'r', 'e'
                , 'p', 'e', 'a', 't', ']', ' ', 'e', 'l', 'e', 'm', 'e'
                , 'n', 't'} }
    };

    dummy_rule_context * ctx = nullptr;

    for (auto const & item : test_values) {
        auto first = item.data.begin();
        auto last = item.data.end();
        auto pos = first;

        auto result = advance_rule(pos, last, ctx);

        CHECK(result == item.success);
        CHECK(static_cast<int>(std::distance(first, pos)) == item.distance);
    }
}

struct dummy_rulelist_context
    : dummy_rule_context
{};

TEST_CASE("advance_rulelist") {
    using pfs::parser::abnf::advance_rulelist;

    std::vector<test_item> test_values = {
          { true, 4, {' ', ';', '\n', '\n'} }
        , { true, 18, {'w', '=', '"', ' ', '"', ' '
              , '/', ' ', '"', '\\', 't', '"', ';', ' ', 'c', '\n', '\n', '\n'} }
        , { true, 12, {'w', '=', 'a', ' ', '/', 'b', ';', ' ', 'c', '\n', '\n', '\n'} }

        , { true, 32, {'W', 'S', 'P', ' ', '=', ' ', '"', ' ', '"', ' '
              , '/', ' ', '"', '\\', 't', '"', ';', ' ', 'w', 'h', 'i', 't', 'e'
              , ' ', 's', 'p', 'a', 'c', 'e', '\n', '\n', '\n'} }
        , { true, 9, {'r', ' ', '=', ' ', '[', 'p', ']', ' ', 'e'} }

        , { true, 34, {'r', 'e', 'p', 'e', 't', 'i', 't', 'i', 'o', 'n'
                , ' ', ' ', ' ', ' ', ' ', '=', ' ', ' ', '[', 'r', 'e'
                , 'p', 'e', 'a', 't', ']', ' ', 'e', 'l', 'e', 'm', 'e'
                , 'n', 't'} }
    };

    dummy_rulelist_context * ctx = nullptr;

    for (auto const & item : test_values) {
        auto first = item.data.begin();
        auto last = item.data.end();
        auto pos = first;

        auto result = advance_rulelist(pos, last, ctx);

        CHECK(result == item.success);
        CHECK(static_cast<int>(std::distance(first, pos)) == item.distance);
    }
}
