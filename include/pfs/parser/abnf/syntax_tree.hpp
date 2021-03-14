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
#include <stack>
#include <vector>
#include <cassert>

#if __cplusplus < 201402L // before C++14
#include <memory>
#endif

namespace pfs {
namespace parser {
namespace abnf {

#if PFS_SYNTAX_TREE_TRACE_ENABLE
#   define PFS_SYNTAX_TREE_TRACE(x) x
#else
#   define PFS_SYNTAX_TREE_TRACE(x)
#endif

#if __cplusplus < 201402L // before C++14
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args &&... args)
{
    static_assert(!std::is_array<T>::value, "arrays not supported");
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
#else
    using std::make_unique;
#endif

enum class node_enum
{
      unknown
    , prose
    , number
    , quoted_string
    , comment
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
// comment_node
////////////////////////////////////////////////////////////////////////////////
template <typename StringType>
class comment_node : public string_node<StringType>
{
    using base_class = string_node<StringType>;

public:
    comment_node (StringType && text)
        : base_class(node_enum::comment, std::forward<StringType>(text))
    {}
};

////////////////////////////////////////////////////////////////////////////////
// comment_node
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
// comment_node
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
// rule_spec
////////////////////////////////////////////////////////////////////////////////
// template <typename StringType>
// class rule_spec final
// {
// public:
//     using string_type = StringType;
//
// private:
//     string_type _name;
//
// public:
//     rule_spec () = delete;
//     ~rule_spec () = default;
//
//     rule_spec (rule_spec const &) = delete;
//     rule_spec & operator = (rule_spec const &) = delete;
//
//     rule_spec (rule_spec &&) = default;
//     rule_spec & operator = (rule_spec &&) = default;
// };

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
    using comment_node_type = comment_node<string_type>;
    using rulename_node_type = rulename_node<string_type>;
    using repetition_node_type = repetition_node<string_type>;
    using group_node_type = group_node<string_type>;
    using option_node_type = option_node<string_type>;
    using concatenation_node_type = concatenation_node<string_type>;
    using alternation_node_type = alternation_node<string_type>;
    using rule_node_type = rule_node<string_type>;
    using rulelist_node_type = rulelist_node<string_type>;
    using aggregate_node_type = aggregate_node<string_type>;

    rulelist_node_type * _rulelist {nullptr};
    std::stack<std::unique_ptr<basic_node>> _node_stack;

#if PFS_SYNTAX_TREE_TRACE_ENABLE
    int _indent_level = 0;

    string_type indent ()
    {
        //return string_type{1, '|'} + string_type(_indent_level * 2, '_');

        string_type result {1, '|'};
        auto i = _indent_level;

        while (i--) {
            result += string_type(2, '_');

            if (i > 0)
                result += string_type(1, '|');
        }

        return result;
    }
#endif

private:
    inline aggregate_node_type * check_aggregate_node ()
    {
        assert(!_node_stack.empty());
        auto & node = _node_stack.top();
        assert(node->is_aggregate_node());
        return static_cast<aggregate_node_type *>(& *node);
    }

    inline number_node_type * check_number_node ()
    {
        assert(!_node_stack.empty());
        auto & node = _node_stack.top();
        assert(node->type() == node_enum::number);
        return static_cast<number_node_type *>(& *node);
    }

    inline repetition_node_type * check_repetition_node ()
    {
        assert(!_node_stack.empty());
        auto & node = _node_stack.top();
        assert(node->type() == node_enum::repetition);
        return static_cast<repetition_node_type *>(& *node);
    }

public:
    syntax_tree_context ()
    {
        auto rulelist = make_unique<rulelist_node_type>();
        _rulelist = & *rulelist;
        _node_stack.push(std::move(rulelist));
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
        _node_stack.push(std::move(num));
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

        auto & num = _node_stack.top();
        _node_stack.pop();

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

        // TODO
        auto gr = make_unique<group_node_type>();
        _node_stack.push(std::move(gr));

        PFS_SYNTAX_TREE_TRACE((++_indent_level));
    }

    void end_group (bool success)
    {
        PFS_SYNTAX_TREE_TRACE((--_indent_level));

        // TODO Implement
        assert(!_node_stack.empty());
        _node_stack.pop();

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "END group: " << (success ? "YES" : "NO") << "\n"
        ));
    }

    // OptionContext
    void begin_option ()
    {
        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "BEGIN option\n"
        ));

        // TODO
        auto opt = make_unique<option_node_type>();
        _node_stack.push(std::move(opt));

        PFS_SYNTAX_TREE_TRACE((++_indent_level));
    }

    void end_option (bool success)
    {
        PFS_SYNTAX_TREE_TRACE((--_indent_level));

        // TODO Implement
        assert(!_node_stack.empty());
        _node_stack.pop();

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "END option: " << (success ? "YES" : "NO") << "\n"
        ));
    }

    // CommentContext
    void comment (forward_iterator first, forward_iterator last)
    {
        auto value = string_type(first.base(), last.base());

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "comment: \"" << value << "\"\n"
        ));

        auto cn = check_aggregate_node();
        auto com = make_unique<comment_node_type>(std::move(value));
        cn->push_back(std::move(com));
    }

    // RepeatContext
    void repeat (long from, long to)
    {
        auto cn = check_repetition_node();

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "repeat: " << from << '-' << to << "\n"
        ));

        cn->set_range(from, to);
    }

    // RulenameContext
    void rulename (forward_iterator first, forward_iterator last)
    {
        auto value = string_type(first.base(), last.base());

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "rulename: \"" << value << "\"\n"
        ));

        assert(!_node_stack.empty());
        auto & node = _node_stack.top();

        if (node->type() == node_enum::rule) {
            auto cn = static_cast<rule_node_type *>(& *node);
            cn->set_name(std::move(value));
        } else {
            auto cn = check_repetition_node();
            auto rn = make_unique<rulename_node_type>(std::move(value));
            cn->set_element(std::move(rn));
        }
    }

    // RepetitionContext
    void begin_repetition ()
    {
        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "BEGIN repetition\n"
        ));

        // FIXME
//         auto cn = check_aggregate_node();
        auto rep = make_unique<repetition_node_type>();
        _node_stack.push(std::move(rep));
//         cn->push_back(std::move(rep));

        PFS_SYNTAX_TREE_TRACE((++_indent_level));
    }

    void end_repetition (bool success)
    {
        PFS_SYNTAX_TREE_TRACE((--_indent_level));

        assert(!_node_stack.empty());
        // FIXME
//         auto top = _node_stack.top()
        _node_stack.pop();
//
//         if (success) {
//             _node_stack.pop();
//         } else {
//
//         }

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "END repetition: " << (success ? "YES" : "NO") << "\n"
        ));
    }

    // AlternationContext
    void begin_alternation ()
    {
        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "BEGIN alternation\n"
        ));

        // TODO Implement

        auto alt = make_unique<alternation_node_type>();
        _node_stack.push(std::move(alt));

        PFS_SYNTAX_TREE_TRACE((++_indent_level));
    }

    void end_alternation (bool success)
    {
        PFS_SYNTAX_TREE_TRACE((--_indent_level));

        // TODO Implement
        _node_stack.pop();

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "END alternation: " << (success ? "YES" : "NO") << "\n"
        ));
    }

    // ConcatenationContext
    void begin_concatenation ()
    {
        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "BEGIN concatenation\n"
        ));

        // TODO Implement

        auto cat = make_unique<concatenation_node_type>();
        _node_stack.push(std::move(cat));

        PFS_SYNTAX_TREE_TRACE((++_indent_level));
    }

    void end_concatenation (bool success)
    {
        PFS_SYNTAX_TREE_TRACE((--_indent_level));

        // TODO Implement

        _node_stack.pop();

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "END concatenation: " << (success ? "YES" : "NO") << "\n"
        ));
    }

    // RuleContext
    void begin_rule ()
    {
        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "BEGIN rule\n"
        ));

        // FIXME
//         auto cn = check_aggregate_node();
        auto rule = make_unique<rule_node_type>();
        _node_stack.push(std::move(rule));
//         cn->push_back(std::move(rule));

        PFS_SYNTAX_TREE_TRACE((++_indent_level));
    };

    void end_rule (bool success)
    {
        PFS_SYNTAX_TREE_TRACE((--_indent_level));

        // FIXME
        _node_stack.pop();

        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "END rule: " << (success ? "YES" : "NO") << "\n"
        ));
    }

    // DefinedAsContext
    void accept_basic_rule_definition ()
    {
        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "accept_basic_rule_definition\n"
        ));

        /*rulenames++;*/
        // TODO Implement
    }

    void accept_incremental_alternatives ()
    {
        PFS_SYNTAX_TREE_TRACE((
            std::cout << indent() << "accept_incremental_alternatives\n"
        ));

        // TODO Implement
    }
};


}}} // pfs::parser::abnf
