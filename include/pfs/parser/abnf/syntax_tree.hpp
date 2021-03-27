////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [pfs-parser](https://github.com/semenovf/pfs-parser) library.
//
// Changelog:
//      2021.02.13 Initial version
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "../line_counter_iterator.hpp"
#include "parser.hpp"
#include "node.hpp"
#include <map>
#include <memory>
#include <stack>
#include <string>
#include <cassert>

namespace pfs {
namespace parser {
namespace abnf {

#if PFS_SYNTAX_TREE_TRACE_ENABLE
#   define PFS_SYNTAX_TREE_TRACE(x) x
#else
#   define PFS_SYNTAX_TREE_TRACE(x)
#endif

// NOTE Below #if statement is borrowed from 'abseil-cpp' project.
// Gcc 4.8 has __cplusplus at 201301 but the libstdc++ shipped with it doesn't
// define make_unique.  Other supported compilers either just define __cplusplus
// as 201103 but have make_unique (msvc), or have make_unique whenever
// __cplusplus > 201103 (clang).
#if (__cplusplus > 201103L || defined(_MSC_VER)) && \
    !(defined(__GLIBCXX__) && !defined(__cpp_lib_make_unique))

using std::make_unique;

#else

// Naive implementation of pre-C++14 std::make_unique
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args &&... args)
{
    static_assert(!std::is_array<T>::value, "arrays not supported");
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

#endif

////////////////////////////////////////////////////////////////////////////////
// syntax_tree_cotext
////////////////////////////////////////////////////////////////////////////////
template <typename StringType, typename ForwardIterator>
class syntax_tree
{
public:
    using string_type = StringType;
    using forward_iterator = line_counter_iterator<ForwardIterator>;

private:
    using prose_node_type = prose_node<string_type>;
    using number_node_type = number_node<string_type>;
    using quoted_string_node_type = quoted_string_node<string_type>;
    using rulename_node_type = rulename_node<string_type>;
    using repetition_node_type = repetition_node;
    using group_node_type = group_node;
    using option_node_type = option_node;
    using concatenation_node_type = concatenation_node;
    using alternation_node_type = alternation_node;
    using rule_node_type = rule_node<string_type>;
    using rulelist_node_type = rulelist_node<string_type>;
    using aggregate_node_type = aggregate_node;

private:
    size_t _max_quoted_string_length = 0;
    rulelist_node_type * _rulelist {nullptr};
    std::stack<std::unique_ptr<basic_node>> _stack;

    struct /*error_spec*/
    {
        std::error_code ec;
        int lineno = 0;
        StringType what;
    } _error_spec;

#if PFS_SYNTAX_TREE_TRACE_ENABLE
    int _indent_level = 0;
    int _indent_step = 4;

    string_type indent ()
    {
        //return string_type{1, '|'} + string_type(_indent_level * _indent_step, '_');

        string_type result {1, '|'};
        auto i = _indent_level;

        while (i--) {
            result += string_type(_indent_step, '-');

            if (i > 0)
                result += string_type(1, '|');
        }

        return result;
    }
#endif

private: // Helper methods
    inline aggregate_node_type * check_aggregate_node ()
    {
        assert(!_stack.empty());
        auto & node = _stack.top();
        assert(node->is_aggregate_node());
        return static_cast<aggregate_node_type *>(& *node);
    }

    inline number_node_type * check_number_node ()
    {
        assert(!_stack.empty());
        auto & node = _stack.top();
        assert(node->type() == node_enum::number);
        return static_cast<number_node_type *>(& *node);
    }

    inline repetition_node_type * check_repetition_node ()
    {
        assert(!_stack.empty());
        auto & node = _stack.top();
        assert(node->type() == node_enum::repetition);
        return static_cast<repetition_node_type *>(& *node);
    }

    inline rulelist_node_type * check_rulelist_node ()
    {
        assert(!_stack.empty());
        auto & node = _stack.top();
        assert(node->type() == node_enum::rulelist);
        return static_cast<rulelist_node_type *>(& *node);
    }

    inline basic_node * end_aggregate_component (bool success)
    {
        assert(!_stack.empty());

        basic_node * result {nullptr};
        auto item = std::move(_stack.top());
        _stack.pop();

        if (success) {
            auto cn = check_aggregate_node();
            result = & *item;
            cn->push_back(std::move(item));
        }

        return result;
    }

public:
    syntax_tree (size_t max_quoted_string_length = 0)
        : _max_quoted_string_length(max_quoted_string_length)
    {}

    syntax_tree (syntax_tree && other) = default;
    syntax_tree & operator = (syntax_tree && other) = default;

    std::error_code error_code () const
    {
        return _error_spec.ec;
    }

    int error_line () const
    {
        return _error_spec.lineno;
    }

    string_type error_text () const
    {
        return _error_spec.what;
    }

    size_t rules_count () const
    {
        assert(_rulelist);
        return _rulelist->size();
    }

public: // Parse context requiremenets
    // xLCOV_EXCL_START
    void error (std::error_code ec, forward_iterator near_pos)
    {
        _error_spec.ec = ec;
        _error_spec.lineno = near_pos.lineno();
    }

    void syntax_error (std::error_code ec
        , forward_iterator near_pos
        , string_type const & what)
    {
        _error_spec.ec = ec;
        _error_spec.lineno = near_pos.lineno();
        _error_spec.what = what;
    }
    // xLCOV_EXCL_STOP

    size_t max_quoted_string_length ()
    {
        return _max_quoted_string_length;
    }

    bool begin_document ()
    {
        auto rulelist = make_unique<rulelist_node_type>();
        _rulelist = & *rulelist;
        _stack.push(std::move(rulelist));
        return true;
    }

    bool end_document (bool success)
    {
        assert(_stack.size() == 1);
        return true;
    }

    // ProseContext
    bool prose (forward_iterator first, forward_iterator last)
    {
        auto value = string_type(first.base(), last.base());

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "prose: \"" << value << "\"\n"
        ));

        auto cn = check_repetition_node();
        auto prose = make_unique<prose_node_type>(std::move(value));
        cn->set_element(std::move(prose));

        return true;
    }

    // NumberContext
    bool first_number (number_flag flag
        , forward_iterator first
        , forward_iterator last)
    {
        auto value = string_type(first.base(), last.base());

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "first_number: " <<  value << "\n"
        ));

        auto num = make_unique<number_node_type>(flag);
        num->set_first(std::move(value));
        _stack.push(std::move(num));

        return true;
    }

    bool last_number (number_flag
        , forward_iterator first
        , forward_iterator last)
    {
        auto value = string_type(first.base(), last.base());

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "last_number: " << value << "\n"
        ));

        auto cn = check_number_node();

        // Inequality of iterators is a flag of a range
        // so this is the last number in a range
        if (first != last) {
            cn->set_last(std::move(value));
        } else {
            cn->push_next(std::move(value));
        }

        auto & num = _stack.top();
        _stack.pop();

        auto rep = check_repetition_node();
        rep->set_element(std::move(num));

        return true;
    }

    bool next_number (number_flag
        , forward_iterator first
        , forward_iterator last)
    {
        auto value = string_type(first.base(), last.base());

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "next_number: " << value << "\n"
        ));

        auto cn = check_number_node();
        cn->push_next(std::move(value));

        return true;
    }

    // QuotedStringContext
    bool quoted_string (forward_iterator first, forward_iterator last)
    {
        auto value = string_type(first.base(), last.base());

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "quoted_string: \"" << value << "\"\n"
        ));

        auto cn = check_repetition_node();
        auto qs = make_unique<quoted_string_node_type>(std::move(value));
        cn->set_element(std::move(qs));

        return true;
    }

    // GroupContext
    bool begin_group ()
    {
        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "BEGIN group\n"
        ));

        auto gr = make_unique<group_node_type>();
        _stack.push(std::move(gr));

        PFS_SYNTAX_TREE_TRACE((++_indent_level));

        return true;
    }

    bool end_group (bool success)
    {
        PFS_SYNTAX_TREE_TRACE((--_indent_level));

        assert(!_stack.empty());

        auto gr = std::move(_stack.top());
        _stack.pop();

        if (success) {
            auto cn = check_repetition_node();
            cn->set_element(std::move(gr));
        }

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "END group: " << (success ? "SUCCESS" : "FAILED") << "\n"
        ));

        return true;
    }

    // OptionContext
    bool begin_option ()
    {
        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "BEGIN option\n"
        ));

        auto opt = make_unique<option_node_type>();
        _stack.push(std::move(opt));

        PFS_SYNTAX_TREE_TRACE((++_indent_level));

        return true;
    }

    bool end_option (bool success)
    {
        PFS_SYNTAX_TREE_TRACE((--_indent_level));

        assert(!_stack.empty());

        auto opt = std::move(_stack.top());
        _stack.pop();

        if (success) {
            auto cn = check_repetition_node();
            cn->set_element(std::move(opt));
        }

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "END option: " << (success ? "SUCCESS" : "FAILED") << "\n"
        ));

        return true;
    }

    // RepeatContext
    bool repeat (long from, long to)
    {
        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "repeat: " << from << '-' << to << "\n"
        ));

        auto cn = check_repetition_node();
        cn->set_range(from, to);

        return true;
    }

    // RulenameContext
    bool rulename (forward_iterator first, forward_iterator last)
    {
        auto value = string_type(first.base(), last.base());

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "rulename: \"" << value << "\"\n"
        ));

        auto cn = check_repetition_node();
        auto rn = make_unique<rulename_node_type>(std::move(value));
        cn->set_element(std::move(rn));

        return true;
    }

    // RepetitionContext
    bool begin_repetition ()
    {
        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "BEGIN repetition\n"
        ));

        auto rep = make_unique<repetition_node_type>();
        _stack.push(std::move(rep));

        PFS_SYNTAX_TREE_TRACE((++_indent_level));

        return true;
    }

    bool end_repetition (bool success)
    {
        PFS_SYNTAX_TREE_TRACE((--_indent_level));

        end_aggregate_component(success);

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "END repetition: " << (success ? "SUCCESS" : "FAILED") << "\n"
        ));

        return true;
    }

    // AlternationContext
    bool begin_alternation ()
    {
        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "BEGIN alternation\n"
        ));

        auto alt = make_unique<alternation_node_type>();
        _stack.push(std::move(alt));

        PFS_SYNTAX_TREE_TRACE((++_indent_level));

        return true;
    }

    bool end_alternation (bool success)
    {
        PFS_SYNTAX_TREE_TRACE((--_indent_level));

        end_aggregate_component(success);

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "END alternation: " << (success ? "SUCCESS" : "FAILED") << "\n"
        ));

        return true;
    }

    // ConcatenationContext
    bool begin_concatenation ()
    {
        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "BEGIN concatenation\n"
        ));

        auto cat = make_unique<concatenation_node_type>();
        _stack.push(std::move(cat));

        PFS_SYNTAX_TREE_TRACE((++_indent_level));

        return true;
    }

    bool end_concatenation (bool success)
    {
        PFS_SYNTAX_TREE_TRACE((--_indent_level));

        end_aggregate_component(success);

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "END concatenation: " << (success ? "SUCCESS" : "FAILED") << "\n"
        ));

        return true;
    }

    bool begin_rule (forward_iterator first, forward_iterator last
        , bool is_incremental_alternatives)
    {
        auto value = string_type(first.base(), last.base());

        auto cn = check_rulelist_node();
        auto extracted = cn->extract(value);

        if (is_incremental_alternatives) {
            PFS_SYNTAX_TREE_TRACE((
                std::cout << indent() << "BEGIN incremental alternative rule: " << value << "\n"
            ));

            // Error: rule not found
            if (extracted.first == false) {
                syntax_error(make_error_code(errc::rule_undefined), first, value);
                return false;
            }

            _stack.push(std::move(extracted.second));
        } else {
            PFS_SYNTAX_TREE_TRACE((
                std::cout << indent() << "BEGIN basic rule definition: " << value << "\n"
            ));

            // Error: rule already exists
            if (extracted.first == true) {
                syntax_error(make_error_code(errc::rulename_duplicated), first, value);
                return false;
            }

            auto rule = make_unique<rule_node_type>();
            _stack.push(std::move(rule));
        }

        PFS_SYNTAX_TREE_TRACE((++_indent_level));

        return true;
    };

    bool end_rule (forward_iterator first, forward_iterator last
        , bool is_incremental_alternatives
        , bool success)
    {
        auto value = string_type(first.base(), last.base());

        PFS_SYNTAX_TREE_TRACE((--_indent_level));

        assert(!_stack.empty());

        auto node = std::move(_stack.top());
        _stack.pop();

        assert(node->type() == node_enum::rule);

        if (success) {
            auto cn = check_rulelist_node();
            cn->emplace(std::move(value), std::move(node));
        }

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "END rule: " << value
                << ": " << (success ? "SUCCESS" : "FAILED") << "\n"
        ));

        return true;
    }
};

template <typename StringType, typename ForwardIterator>
inline syntax_tree<StringType, ForwardIterator> parse (
      ForwardIterator & first
    , ForwardIterator last)
{
    using context_type = syntax_tree<StringType, ForwardIterator>;
    using forward_iterator = typename context_type::forward_iterator;
    forward_iterator f(first);
    forward_iterator l(last);
    context_type ctx;

    advance_rulelist(f, l, ctx);
    first = f.base();
    return std::move(ctx);
}

template <typename StringType, typename ForwardIterator>
inline syntax_tree<StringType, ForwardIterator> parse_source (
    StringType const & source)
{
    return parse(source.begin(), source.end());
}

}}} // pfs::parser::abnf
