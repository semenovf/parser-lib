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
    void prose (forward_iterator, forward_iterator) {}
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
    void first_number (pfs::parser::abnf::number_flag, forward_iterator, forward_iterator) {}
    void last_number (pfs::parser::abnf::number_flag, forward_iterator, forward_iterator) {}
    void next_number (pfs::parser::abnf::number_flag, forward_iterator, forward_iterator) {}
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

struct dummy_repeat_context
{
    void repeat (forward_iterator first_from, forward_iterator last_from
        , forward_iterator first_to, forward_iterator last_to)
    {}
};

struct repeat_context
{
    using forward_iterator = std::string::const_iterator;

    std::string from;
    std::string to;

    void repeat (forward_iterator first_from, forward_iterator last_from
        , forward_iterator first_to, forward_iterator last_to)
    {
        from = std::string{first_from, last_from};
        to = std::string{first_to, last_to};
    }
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
        CHECK(ctx.from == std::string{"10"});
        CHECK(ctx.to == ctx.from);
    }

    {
        repeat_context ctx;
        std::string s {"10*"};
        auto pos = s.begin();
        CHECK(advance_repeat(pos, s.end(), & ctx));
        CHECK(pos == s.end());
        CHECK(ctx.from == std::string{"10"});
        CHECK(ctx.to.empty());
    }

    {
        repeat_context ctx;
        std::string s {"*10"};
        auto pos = s.begin();
        CHECK(advance_repeat(pos, s.end(), & ctx));
        CHECK(pos == s.end());
        CHECK(ctx.from.empty());
        CHECK(ctx.to == std::string{"10"});
    }

    {
        repeat_context ctx;
        std::string s {"*"};
        auto pos = s.begin();
        CHECK(advance_repeat(pos, s.end(), & ctx));
        CHECK(pos == s.end());
        CHECK(ctx.from.empty());
        CHECK(ctx.to.empty());
    }

    {
        repeat_context ctx;
        std::string s {"10*20"};
        auto pos = s.begin();
        CHECK(advance_repeat(pos, s.end(), & ctx));
        CHECK(pos == s.end());
        CHECK(ctx.from == std::string{"10"});
        CHECK(ctx.to == std::string{"20"});
    }

    {
        repeat_context ctx;
        std::string s {"*x"};
        auto pos = s.begin();
        CHECK(advance_repeat(pos, s.end(), & ctx));
        CHECK_FALSE(pos == s.end());
        CHECK(ctx.from.empty());
        CHECK(ctx.to.empty());
    }
}

struct dummy_comment_context
{
    void comment (forward_iterator first, forward_iterator last) {}
};

struct comment_context
{
    using forward_iterator = std::string::const_iterator;

    std::string text;

    void comment (forward_iterator first, forward_iterator last)
    {
        text = std::string{first, last};
    }
};

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
        dummy_comment_context * ctx = nullptr;

        for (auto const & item: valid_values) {
            auto pos = item.begin();
            CHECK(advance_comment(pos, item.end(), ctx));
            CHECK(pos == item.end());
        }

        for (auto const & item: invalid_values) {
            auto pos = item.begin();
            CHECK_FALSE(advance_comment(pos, item.end(), ctx));
        }
    }

    {
        comment_context ctx;
        std::string s {";"};
        auto pos = s.begin();
        CHECK(advance_comment(pos, s.end(), & ctx));
        CHECK(pos == s.end());
        CHECK(ctx.text.empty());
    }

    {
        comment_context ctx;
        std::string s {";\r\n"};
        auto pos = s.begin();
        CHECK(advance_comment(pos, s.end(), & ctx));
        CHECK(pos == s.end());
        CHECK(ctx.text.empty());
    }

    {
        comment_context ctx;
        std::string s {"; comment "};
        auto pos = s.begin();
        CHECK(advance_comment(pos, s.end(), & ctx));
        CHECK(pos == s.end());
        CHECK(ctx.text == std::string{" comment "});
    }

    {
        comment_context ctx;
        std::string s {"; comment \r\n"};
        auto pos = s.begin();
        CHECK(advance_comment(pos, s.end(), & ctx));
        CHECK(pos == s.end());
        CHECK(ctx.text == std::string{" comment "});
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
        dummy_comment_context * ctx = nullptr;

        for (auto const & item: valid_values) {
            auto pos = item.begin();
            CHECK(advance_comment_newline(pos, item.end(), ctx));
            CHECK(pos == item.end());
        }

        for (auto const & item: invalid_values) {
            auto pos = item.begin();
            CHECK_FALSE(advance_comment_newline(pos, item.end(), ctx));
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
        dummy_comment_context * ctx = nullptr;

        for (auto const & item: valid_values) {
            auto pos = item.begin();
            CHECK(advance_comment_whitespace(pos, item.end(), ctx));
            CHECK(pos == item.end());
        }

        for (auto const & item: invalid_values) {
            auto pos = item.begin();
            CHECK_FALSE(advance_comment_whitespace(pos, item.end(), ctx));
        }
    }
}

struct dummy_rulename_context
{
    void rulename (forward_iterator first, forward_iterator last) {}
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
    : dummy_rulename_context
    , dummy_number_context
    , dummy_quoted_string_context
    , dummy_prose_context
{};

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
    , dummy_repeat_context
{
    void begin_repetition () {}
    void end_repetition (bool) {}
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

// struct dummy_concatenation_context: dummy_repetition_context
// {};
//
//
// TEST_CASE("advance_concatenation") {
//     using pfs::parser::abnf::advance_concatenation;
//
//     std::vector<char> valid_values[0]/* = {
//         // Prose
//           {'<', '>'}
//         , {'*', '<', '>'}
//         , {'1', '*', '<', ' ', '>'}
//         , {'*', '2', '<', '\x20', '>'}
//         , {'1', '*', '2', '<', ' ', '\x3D', ' ', '>'}
//
//         // Number
//         , {'%', 'b', '0', '1', '1'}                     // %b001
//         , {'*', '%', 'b', '0', '1', '1'}                // %b001
//         , {'1', '*', '%', 'b', '0', '-', '1'}           // %b0-1
//         , {'*', '2', '%', 'b', '0', '0', '-', '1', '1'} // %b00-11
//         , {'1', '*', '2', '%', 'b', '0', '.', '1', '.', '1', '1'} // %b0.1.11
//
//         // Quoted string
//         , {'"', '"'}
//         , {'*', '"', '"'}
//         , {'1', '*', '"', ' ', '"'}
//         , {'*', '2', '"', '\x21', ' ', '\x7E', '"'}
//         , {'1', '*', '2', '"', '\x21', ' ', '\x7E', '"'}
//
//         // Rulename
//         , {'A'}
//         , {'*', 'A'}
//         , {'1', '*', 'A', '1'}
//         , {'*', '2', 'A', '1', '-'}
//         , {'1', '*', '2', 'A', '-', '1'}
//     }*/;
//
//     std::vector<char> invalid_values[0]/* = {
//           {' '}
//     }*/;
//
//     {
//         dummy_concatenation_context * ctx = nullptr;
//
//         for (auto const & item : valid_values) {
//             auto pos = item.begin();
//             CHECK(advance_concatenation(pos, item.end(), ctx));
//             CHECK(pos == item.end());
//         }
//
//         for (auto const & item : invalid_values) {
//             auto pos = item.begin();
//             CHECK_FALSE(advance_concatenation(pos, item.end(), ctx));
//         }
//     }
// }
