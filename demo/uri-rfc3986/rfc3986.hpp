////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [pfs-parser](https://github.com/semenovf/pfs-parser) library.
//
// Changelog:
//      2021.02.25 Initial version
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/parser/abnf/parser.hpp"
#include "pfs/parser/line_counter_iterator.hpp"

namespace grammar {
namespace rfc3986 {

using forward_iterator = pfs::parser::line_counter_iterator<std::string::const_iterator>;

class context
{
    int rulenames = 0;
    std::string _lastError;

public:
    std::string lastError () const
    {
        return _lastError;
    }

    bool begin_document () { return true; }
    bool end_document (bool) { return true; }

    // ProseContext
    bool prose (forward_iterator, forward_iterator) { return true; }

    // NumberContext
    bool first_number (pfs::parser::abnf::number_flag, forward_iterator, forward_iterator) { return true; }
    bool last_number (pfs::parser::abnf::number_flag, forward_iterator, forward_iterator) { return true; }
    bool next_number (pfs::parser::abnf::number_flag, forward_iterator, forward_iterator) { return true; }

    // QuotedStringContext
    bool quoted_string (forward_iterator, forward_iterator) { return true; }

    void error (std::error_code ec, forward_iterator near_pos)
    {
        _lastError = ec.message();
        _lastError += " at ";
        _lastError += std::to_string(near_pos.lineno());
        _lastError += " line";
    }

    size_t max_quoted_string_length () { return 0; }

    // GroupContext
    bool begin_group () { return true; }
    bool end_group (bool success) { return true; }

    // OptionContext
    bool begin_option () { return true; }
    bool end_option (bool success) { return true; }

    // RepeatContext
    bool repeat (long from, long to) { return true; }

    // RulenameContext
    bool rulename (forward_iterator first, forward_iterator last) { return true; }

    // RepetitionContext
    bool begin_repetition () { return true; }
    bool end_repetition (bool success) { return true; }

    // AlternationContext
    bool begin_alternation () { return true; }
    bool end_alternation (bool success) { return true; }

    // ConcatenationContext
    bool begin_concatenation () { return true; }
    bool end_concatenation (bool success) { return true; }

    // RuleContext
    bool begin_rule (forward_iterator, forward_iterator
        , bool is_incremental_alternatives)
    {
        if (!is_incremental_alternatives)
            rulenames++;
        return true;
    }

    bool end_rule (forward_iterator, forward_iterator, bool, bool) { return true; }
};

}} // grammar::rfc3986
