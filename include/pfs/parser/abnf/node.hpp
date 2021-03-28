////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [pfs-parser](https://github.com/semenovf/pfs-parser) library.
//
// Changelog:
//      2021.02.22 Initial version (cut from syntax_tree.hpp)
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <map>
#include <vector>
#include <cassert>

namespace pfs {
namespace parser {
namespace abnf {

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
            || _type == node_enum::alternation;
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

    StringType text () const
    {
        return _text;
    }
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

    bool is_range () const
    {
        return _is_range;
    }

    typename std::vector<StringType>::const_iterator begin () const
    {
        return _value.begin();
    }

    typename std::vector<StringType>::const_iterator end () const
    {
        return _value.end();
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

    basic_node const * element () const
    {
        return & *_element;
    }
};

////////////////////////////////////////////////////////////////////////////////
// aggregate_node
////////////////////////////////////////////////////////////////////////////////
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
    void visit (Visitor && vis) const
    {
        for (auto & item: _siblings) {
            vis(& *item);
        }
    }
};

////////////////////////////////////////////////////////////////////////////////
// rule_node
////////////////////////////////////////////////////////////////////////////////
template <typename StringType>
class rule_node : public aggregate_node
{
    using base_class = aggregate_node;

    StringType _name;

public:
    rule_node ()
        : base_class(node_enum::rule)
    {}

    void set_name (StringType && name)
    {
        _name = std::forward<StringType>(name);
    }

    StringType name () const
    {
        return _name;
    }
};

////////////////////////////////////////////////////////////////////////////////
// group_node
////////////////////////////////////////////////////////////////////////////////
class group_node : public aggregate_node
{
    using base_class = aggregate_node;

public:
    group_node ()
        : base_class(node_enum::group)
    {}
};

////////////////////////////////////////////////////////////////////////////////
// option_node
////////////////////////////////////////////////////////////////////////////////
class option_node : public aggregate_node
{
    using base_class = aggregate_node;

public:
    option_node ()
        : base_class(node_enum::option)
    {}
};

////////////////////////////////////////////////////////////////////////////////
// concatenation_node
////////////////////////////////////////////////////////////////////////////////
class concatenation_node : public aggregate_node
{
    using base_class = aggregate_node;

public:
    concatenation_node ()
        : base_class(node_enum::concatenation)
    {}
};

////////////////////////////////////////////////////////////////////////////////
// alternation_node
////////////////////////////////////////////////////////////////////////////////
class alternation_node : public aggregate_node
{
    using base_class = aggregate_node;

public:
    alternation_node ()
        : base_class(node_enum::alternation)
    {}
};

////////////////////////////////////////////////////////////////////////////////
// rulelist_node
////////////////////////////////////////////////////////////////////////////////
template <typename StringType>
class rulelist_node : public basic_node
{
    using base_class = basic_node;
    using string_type = StringType;
    using item_type = std::unique_ptr<basic_node>;
    using container = std::map<string_type, item_type>;

public:
    using iterator = typename container::iterator;
    using const_iterator = typename container::const_iterator;

private:
    container _rules;

public:
    rulelist_node ()
        : base_class(node_enum::rulelist)
    {}

    void emplace (string_type && name, item_type && item)
    {
        _rules.emplace(std::forward<string_type>(name)
            , std::forward<item_type>(item));
    }

    std::pair<bool, item_type> extract (string_type const & name)
    {
        auto it = _rules.find(name);

        if (it == _rules.end())
            return std::make_pair(false, item_type{});

        auto result = std::make_pair(true, std::move(it->second));
        _rules.erase(it);
        return result;
    }

    size_t size () const
    {
        return _rules.size();
    }

    template <typename Visitor>
    void visit (Visitor && vis) const
    {
        for (auto & item: _rules) {
            vis(& *item.second);
        }
    }
};

}}} // pfs::parser::abnf
