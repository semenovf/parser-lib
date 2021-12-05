////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [pfs-parser](https://github.com/semenovf/pfs-parser) library.
//
// Changelog:
//      2021.02.14 Initial version
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define PFS_SYNTAX_TREE_TRACE_ENABLE 0
#include "doctest.h"
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
    , { "data/number.grammar", 1 }
    , { "data/incremental-alternatives.grammar", 1 }
    , { "data/abnf.grammar", 37 }
    , { "data/json-rfc4627.grammar", 30 }
    , { "data/json-rfc8259.grammar", 30 }
    , { "data/uri-rfc3986.grammar", 36 }
    , { "data/uri-geo-rfc58070.grammar", 27 }
};

using string_type = std::string;
// using forward_iterator = pfs::parser::line_counter_iterator<string_type::const_iterator>;

struct visitor
{
    int _indent_level = 0;
    int _indent_step = 4;

    string_type indent ()
    {
        string_type result {1, '|'};
        auto i = _indent_level;

        while (i--) {
            result += string_type(_indent_step, '-');

            if (i > 0)
                result += string_type(1, '|');
        }

        return result;
    }

    void prose (string_type const & text)
    {
        std::cout << indent() << "PROSE: \"" << text << "\"" << std::endl;
    }

    void number_range (string_type const & from, string_type const & to)
    {
        std::cout << indent() << "NUMBER RANGE: " << from << " - " << to << std::endl;
    }

    void number (string_type const & text)
    {
        std::cout << indent() << "NUMBER: " << text << std::endl;
    }

    void quoted_string (string_type const & text)
    {
        std::cout << indent() << "QUOTED STRING: \"" << text << "\"" << std::endl;
    }

    void rulename (string_type const & text)
    {
        std::cout << indent() << "RULENAME: \"" << text << "\"" << std::endl;
    }

    void begin_repetition ()
    {
        std::cout << indent() << "BEGIN REPETITION" << std::endl;
        ++_indent_level;
    }

    void end_repetition ()
    {
        --_indent_level;
        std::cout << indent() << "END REPETITION" << std::endl;
    }

    void begin_group ()
    {
        std::cout << indent() << "BEGIN GROUP" << std::endl;
        ++_indent_level;
    }

    void end_group ()
    {
        --_indent_level;
        std::cout << indent() << "END GROUP" << std::endl;
    }

    void begin_option ()
    {
        std::cout << indent() << "BEGIN OPTION" << std::endl;
        ++_indent_level;
    }

    void end_option ()
    {
        --_indent_level;
        std::cout << indent() << "END OPTION" << std::endl;
    }

    void begin_concatenation ()
    {
        std::cout << indent() << "BEGIN CONCATENATION" << std::endl;
        ++_indent_level;
    }

    void end_concatenation ()
    {
        --_indent_level;
        std::cout << indent() << "END CONCATENATION" << std::endl;
    }

    void begin_alternation ()
    {
        std::cout << indent() << "BEGIN ALTERNATION" << std::endl;
        ++_indent_level;
    }

    void end_alternation ()
    {
        --_indent_level;
        std::cout << indent() << "END ALTERNATION" << std::endl;
    }

    void begin_rule (string_type const & name)
    {
        std::cout << indent() << "BEGIN RULE: \"" << name << "\"" << std::endl;
        ++_indent_level;
    }

    void end_rule ()
    {
        --_indent_level;
        std::cout << indent() << "END RULE" << std::endl;
    }

    void begin_document ()
    {
        std::cout << indent() << "BEGIN DOCUMENT" << std::endl;
        ++_indent_level;
    }

    void end_document ()
    {
        --_indent_level;
        std::cout << indent() << "END DOCUMENT" << std::endl;
    }
};

TEST_CASE("Syntax Tree") {
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

        auto first = source.begin();
        auto last  = source.end();
        auto st = pfs::parser::abnf::parse<std::string>(first, last);

        if (st.error_code()) {
            std::cerr << "ERROR: parse failed at line "
                << st.error_line()
                << ": " << st.error_code();

            if (!st.error_text().empty())
                std::cerr << ": " << st.error_text();

            std::cerr << std::endl;
        } else {
            visitor v;
            st.traverse(std::move(v));
        }

        if (first != last)
            std::cerr << "ERROR: parse is incomplete" << std::endl;

        CHECK_MESSAGE(!st.error_code(), item.filename);
        CHECK((first == last));
        CHECK(st.rules_count() == item.rulenames);

        return true;
    });

    CHECK_MESSAGE(result, "Parse files tested successfuly");
}
