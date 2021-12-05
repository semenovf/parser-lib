////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [pfs-parser](https://github.com/semenovf/pfs-parser) library.
//
// Changelog:
//      2021.02.25 Initial version
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <string>
#include <iostream>
#include <vector>

namespace grammar {
namespace rfc3986 {

using string_type = std::string;

struct visitor
{
    std::vector<string_type> _rules;

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
        _rules.emplace_back(string_type{name});
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

        int rule_counter = 1;

        std::cout << "\n";
        std::cout
            << R"(////////////////////////////////////////////////////////////////////////////////)" << '\n'
            << R"(// THIS FILE GENERATED AUTOMATICALLY BY pfs-parser (C) generator                )" << '\n'
            << R"(// DATE: )" << __DATE__ << "\n"
            << R"(// TIME: )" << __TIME__ << "\n"
            << R"(////////////////////////////////////////////////////////////////////////////////)" << '\n';

        for (auto const & r: _rules) {
            std::cout << rule_counter++ << '.' << ' ' << r << std::endl;
        }
    }
};

}} // grammar::rfc3986
