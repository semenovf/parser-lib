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
#include <map>
#include <stack>
#include <vector>
#include <cassert>
#include <memory>

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

enum class node_enum
{
      unknown
    , prose
    , number
    , quoted_string
    , rulename
    , repetition
    , group
    , option
    , concatenation
    , alternation
    , rule
    , rulelist
};

////////////////////////////////////////////////////////////////////////////////
// node
////////////////////////////////////////////////////////////////////////////////
class basic_node
{
    node_enum _type {node_enum::unknown};

public:
    basic_node (node_enum type) : _type(type) {}

    node_enum type () const noexcept
    {
        return _type;
    }

    bool is_aggregate_node () const noexcept
    {
        return _type == node_enum::rule
            || _type == node_enum::group
            || _type == node_enum::option
            || _type == node_enum::concatenation
            || _type == node_enum::alternation
            || _type == node_enum::rulelist;
    }
};

////////////////////////////////////////////////////////////////////////////////
// string_node
////////////////////////////////////////////////////////////////////////////////
template <typename StringType>
class string_node : public basic_node
{
protected:
    StringType _text;

protected:
    string_node (node_enum type, StringType && text)
        : basic_node(type)
        , _text(std::forward<StringType>(text))
    {}

public:
    string_node (string_node const &) = delete;
    string_node & operator = (string_node const &) = delete;

    string_node (string_node && other)
        : basic_node(node_enum::prose)
        , _text(std::forward<StringType>(other._text))
    {}

    string_node & operator = (string_node && other) = delete;
};

////////////////////////////////////////////////////////////////////////////////
// prose_node
////////////////////////////////////////////////////////////////////////////////
template <typename StringType>
class prose_node : public string_node<StringType>
{
    using base_class = string_node<StringType>;

public:
    prose_node (StringType && text)
        : base_class(node_enum::prose, std::forward<StringType>(text))
    {}
};

////////////////////////////////////////////////////////////////////////////////
// number_node
////////////////////////////////////////////////////////////////////////////////
template <typename StringType>
class number_node : public basic_node
{
    using base_class = basic_node;

    number_flag _flag {number_flag::unspecified};
    bool _is_range {false};
    std::vector<StringType> _value;

public:
    number_node (number_flag flag)
        : base_class(node_enum::number)
        , _flag(flag)
    {}

    // Set first in a range or a sequence
    void set_first (StringType && text)
    {
        assert(_value.size() == 0);
        _value.push_back(std::forward<StringType>(text));
    }

    // Set last in a range
    void set_last (StringType && text)
    {
        assert(_value.size() == 1);
        _is_range = true;
        _value.push_back(std::forward<StringType>(text));
    }

    void push_next (StringType && text)
    {
        assert(_value.size() > 0);
        assert(_is_range != true);
        _value.push_back(std::forward<StringType>(text));
    }
};

////////////////////////////////////////////////////////////////////////////////
// quoted_string_node
////////////////////////////////////////////////////////////////////////////////
template <typename StringType>
class quoted_string_node : public string_node<StringType>
{
    using base_class = string_node<StringType>;

public:
    quoted_string_node (StringType && text)
        : base_class(node_enum::quoted_string, std::forward<StringType>(text))
    {}
};


////////////////////////////////////////////////////////////////////////////////
// rulename_node
////////////////////////////////////////////////////////////////////////////////
template <typename StringType>
class rulename_node : public string_node<StringType>
{
    using base_class = string_node<StringType>;

public:
    rulename_node (StringType && text)
        : base_class(node_enum::rulename, std::forward<StringType>(text))
    {}
};

////////////////////////////////////////////////////////////////////////////////
// repetition_node
////////////////////////////////////////////////////////////////////////////////
template <typename StringType>
class repetition_node : public basic_node
{
    using base_class = basic_node;

    long _from {1};
    long _to {1};
    std::unique_ptr<basic_node> _element;

public:
    repetition_node ()
        : base_class(node_enum::repetition)
        , _from(1)
        , _to(1)
    {}

    void set_range (long from, long to)
    {
        _from = from;
        _to = to;
    }

    void set_element (std::unique_ptr<basic_node> && elem)
    {
        _element = std::move(elem);
    }
};

////////////////////////////////////////////////////////////////////////////////
// aggregate_node
////////////////////////////////////////////////////////////////////////////////
template <typename StringType>
class aggregate_node : public basic_node
{
    using base_class = basic_node;
    using item_type = std::unique_ptr<basic_node>;
    using size_type = typename std::vector<item_type>::size_type;

protected:
    std::vector<item_type> _siblings;

protected:
    aggregate_node (node_enum type)
        : base_class(type)
    {}

public:
    size_type size () const
    {
        return _siblings.size();
    }

    void push_back (item_type && item)
    {
        _siblings.push_back(std::forward<item_type>(item));
    }

    template <typename Visitor>
    void visit (Visitor && vis)
    {
        for (auto & item: _siblings) {
            vis(item);
        }
    }
};

////////////////////////////////////////////////////////////////////////////////
// rule_node
////////////////////////////////////////////////////////////////////////////////
template <typename StringType>
class rule_node : public aggregate_node<StringType>
{
    using base_class = aggregate_node<StringType>;

    StringType _name;

public:
    rule_node ()
        : base_class(node_enum::rule)
    {}

    void set_name (StringType && name)
    {
        _name = std::forward<StringType>(name);
    }
};

////////////////////////////////////////////////////////////////////////////////
// group_node
////////////////////////////////////////////////////////////////////////////////
template <typename StringType>
class group_node : public aggregate_node<StringType>
{
    using base_class = aggregate_node<StringType>;

public:
    group_node ()
        : base_class(node_enum::group)
    {}
};

////////////////////////////////////////////////////////////////////////////////
// option_node
////////////////////////////////////////////////////////////////////////////////
template <typename StringType>
class option_node : public aggregate_node<StringType>
{
    using base_class = aggregate_node<StringType>;

public:
    option_node ()
        : base_class(node_enum::option)
    {}
};

////////////////////////////////////////////////////////////////////////////////
// concatenation_node
////////////////////////////////////////////////////////////////////////////////
template <typename StringType>
class concatenation_node : public aggregate_node<StringType>
{
    using base_class = aggregate_node<StringType>;

public:
    concatenation_node ()
        : base_class(node_enum::concatenation)
    {}
};

////////////////////////////////////////////////////////////////////////////////
// alternation_node
////////////////////////////////////////////////////////////////////////////////
template <typename StringType>
class alternation_node : public aggregate_node<StringType>
{
    using base_class = aggregate_node<StringType>;

public:
    alternation_node ()
        : base_class(node_enum::alternation)
    {}
};

////////////////////////////////////////////////////////////////////////////////
// rulelist_node
////////////////////////////////////////////////////////////////////////////////
template <typename StringType>
class rulelist_node : public aggregate_node<StringType>
{
    using base_class = aggregate_node<StringType>;

public:
    rulelist_node ()
        : base_class(node_enum::rulelist)
    {}
};

////////////////////////////////////////////////////////////////////////////////
// syntax_tree_cotext
////////////////////////////////////////////////////////////////////////////////
template <typename ForwardIterator, typename StringType>
class syntax_tree_context
{
    using forward_iterator = line_counter_iterator<ForwardIterator>;
    using string_type = StringType;
    using prose_node_type = prose_node<string_type>;
    using number_node_type = number_node<string_type>;
    using quoted_string_node_type = quoted_string_node<string_type>;
    using rulename_node_type = rulename_node<string_type>;
    using repetition_node_type = repetition_node<string_type>;
    using group_node_type = group_node<string_type>;
    using option_node_type = option_node<string_type>;
    using concatenation_node_type = concatenation_node<string_type>;
    using alternation_node_type = alternation_node<string_type>;
    using rule_node_type = rule_node<string_type>;
    using rulelist_node_type = rulelist_node<string_type>;
    using aggregate_node_type = aggregate_node<string_type>;

private:
    rulelist_node_type * _rulelist {nullptr};
    std::stack<std::unique_ptr<basic_node>> _stack;

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

private:
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

    inline void end_aggregate_component (bool success)
    {
        assert(!_stack.empty());

        auto item = std::move(_stack.top());
        _stack.pop();

        if (success) {
            auto cn = check_aggregate_node();
            cn->push_back(std::move(item));
        }
    }

public:
    syntax_tree_context ()
    {
        auto rulelist = make_unique<rulelist_node_type>();
        _rulelist = & *rulelist;
        _stack.push(std::move(rulelist));
    }

    size_t rules_count () const
    {
        assert(_rulelist);
        return _rulelist->size();
    }

    // LCOV_EXCL_START
    void error (std::error_code ec)
    {
        std::cerr << "Parse error: " << ec.message() << std::endl;
    }
    // LCOV_EXCL_STOP

    size_t max_quoted_string_length ()
    {
        return 0;
    }

    // ProseContext
    void prose (forward_iterator first, forward_iterator last)
    {
        auto value = string_type(first.base(), last.base());

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "prose: \"" << value << "\"\n"
        ));

        auto cn = check_repetition_node();
        auto prose = make_unique<prose_node_type>(std::move(value));
        cn->set_element(std::move(prose));
    }

    // NumberContext
    void first_number (pfs::parser::abnf::number_flag flag
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
    }

    void last_number (pfs::parser::abnf::number_flag
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
    }

    void next_number (pfs::parser::abnf::number_flag
        , forward_iterator first
        , forward_iterator last)
    {
        auto value = string_type(first.base(), last.base());

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "next_number: " << value << "\n"
        ));

        auto cn = check_number_node();
        cn->push_next(std::move(value));
    }

    // QuotedStringContext
    void quoted_string (forward_iterator first, forward_iterator last)
    {
        auto value = string_type(first.base(), last.base());

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "quoted_string: \"" << value << "\"\n"
        ));

        auto cn = check_repetition_node();
        auto qs = make_unique<quoted_string_node_type>(std::move(value));
        cn->set_element(std::move(qs));
    }

    // GroupContext
    void begin_group ()
    {
        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "BEGIN group\n"
        ));

        auto gr = make_unique<group_node_type>();
        _stack.push(std::move(gr));

        PFS_SYNTAX_TREE_TRACE((++_indent_level));
    }

    void end_group (bool success)
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
    }

    // OptionContext
    void begin_option ()
    {
        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "BEGIN option\n"
        ));

        auto opt = make_unique<option_node_type>();
        _stack.push(std::move(opt));

        PFS_SYNTAX_TREE_TRACE((++_indent_level));
    }

    void end_option (bool success)
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
    }

    // RepeatContext
    void repeat (long from, long to)
    {
        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "repeat: " << from << '-' << to << "\n"
        ));

        auto cn = check_repetition_node();
        cn->set_range(from, to);
    }

    // RulenameContext
    void rulename (forward_iterator first, forward_iterator last)
    {
        auto value = string_type(first.base(), last.base());

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "rulename: \"" << value << "\"\n"
        ));

        auto cn = check_repetition_node();
        auto rn = make_unique<rulename_node_type>(std::move(value));
        cn->set_element(std::move(rn));
    }

    // RepetitionContext
    void begin_repetition ()
    {
        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "BEGIN repetition\n"
        ));

        auto rep = make_unique<repetition_node_type>();
        _stack.push(std::move(rep));

        PFS_SYNTAX_TREE_TRACE((++_indent_level));
    }

    void end_repetition (bool success)
    {
        PFS_SYNTAX_TREE_TRACE((--_indent_level));

        end_aggregate_component(success);

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "END repetition: " << (success ? "SUCCESS" : "FAILED") << "\n"
        ));
    }

    // AlternationContext
    void begin_alternation ()
    {
        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "BEGIN alternation\n"
        ));

        auto alt = make_unique<alternation_node_type>();
        _stack.push(std::move(alt));

        PFS_SYNTAX_TREE_TRACE((++_indent_level));
    }

    void end_alternation (bool success)
    {
        PFS_SYNTAX_TREE_TRACE((--_indent_level));

        end_aggregate_component(success);

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "END alternation: " << (success ? "SUCCESS" : "FAILED") << "\n"
        ));
    }

    // ConcatenationContext
    void begin_concatenation ()
    {
        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "BEGIN concatenation\n"
        ));

        auto cat = make_unique<concatenation_node_type>();
        _stack.push(std::move(cat));

        PFS_SYNTAX_TREE_TRACE((++_indent_level));
    }

    void end_concatenation (bool success)
    {
        PFS_SYNTAX_TREE_TRACE((--_indent_level));

        end_aggregate_component(success);

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "END concatenation: " << (success ? "SUCCESS" : "FAILED") << "\n"
        ));
    }

    void begin_rule (forward_iterator first, forward_iterator last
        , bool is_incremental_alternatives)
    {
        auto value = string_type(first.base(), last.base());

        if (is_incremental_alternatives) {
            PFS_SYNTAX_TREE_TRACE((
                std::cout << indent() << "BEGIN incremental alternative rule: " << value << "\n"
            ));

            // FIXME Need to design the technique to add incremental alternatives
        } else {
            PFS_SYNTAX_TREE_TRACE((
                std::cout << indent() << "BEGIN basic rule definition: " << value << "\n"
            ));

            auto rule = make_unique<rule_node_type>();
            _stack.push(std::move(rule));
        }

        PFS_SYNTAX_TREE_TRACE((++_indent_level));
    };

    void end_rule (forward_iterator first, forward_iterator last
        , bool is_incremental_alternatives
        , bool success)
    {
        auto value = string_type(first.base(), last.base());

        PFS_SYNTAX_TREE_TRACE((--_indent_level));

        if (! is_incremental_alternatives) {
            end_aggregate_component(success);
        }

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "END rule: " << value
                << ": " << (success ? "SUCCESS" : "FAILED") << "\n"
        ));
    }
};


////////////////////////////////////////////////////////////////////////////////
// syntax_tree
////////////////////////////////////////////////////////////////////////////////

// template <typename StringType>
// class syntax_tree final
// {
//     using rule_spec_type = rule_spec<StringType>;
//
//     std::vector<rule_spec_type> _rules;
//
// public:
//     syntax_tree () = default;
//     ~syntax_tree () = default;
//
//     syntax_tree (syntax_tree const & other) = delete;
//     syntax_tree & operator = (syntax_tree const & other) = delete;
//
//     syntax_tree (syntax_tree && other) = default;
//     syntax_tree & operator = (syntax_tree && other) = default;
// };

}}} // pfs::parser::abnf
