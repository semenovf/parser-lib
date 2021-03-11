////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [pfs-parser](https://github.com/semenovf/pfs-parser) library.
//
// Changelog:
//      2021.02.13 Initial version
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <vector>

namespace pfs {
namespace parser {
namespace abnf {

enum class node_enum
{
      unknown
    , prose
};

////////////////////////////////////////////////////////////////////////////////
// node
////////////////////////////////////////////////////////////////////////////////
template <typename StringType>
class basic_node
{
    node_enum _type {node_enum::unknown};

public:
    basic_node (node_enum type) : _type(type) {}

    node_enum type () const noexcept
    {
        return _type;
    }
};

////////////////////////////////////////////////////////////////////////////////
// prose_node
////////////////////////////////////////////////////////////////////////////////
template <typename StringType>
class prose_node : public basic_node
{
    StringType _text;

protected:
    prose_node (StringType const & text)
        : basic_node(node_enum::prose)
        , _text(text)
    {}

    prose_node (StringType && text)
        : basic_node(node_enum::prose)
        , _text(std::forward<StringType>(text))
    {}

public:
    prose_node (prose_node const &) = delete;
    prose_node & operator = (prose_node const &) = delete;

    prose_node (prose_node && other)
        : basic_node(node_enum::prose)
        , _text(std::forward<StringType>(other._text))
    {}

    prose_node & operator = (prose_node && other) = delete;

    template <typename U>
    friend prose_node<U> make_prose (U const & text);

    template <typename U>
    friend prose_node<U> make_prose (U && text);

    template <typename U>
    friend std::unique_ptr<basic_node> make_unique_prose (U const & text);

    template <typename U>
    friend std::unique_ptr<basic_node> make_unique_prose (U && text);
};

template <typename StringType>
inline prose_node<StringType> make_prose (StringType const & text)
{
    return prose_node<StringType>(text);
}

template <typename StringType>
inline prose_node<StringType> make_prose (StringType && text)
{
    return prose_node<StringType>(std::forward<StringType>(text));
}

template <typename StringType>
inline std::unique_ptr<basic_node> make_unique_prose (StringType const & text)
{
    return std::unique_ptr<basic_node>(new prose_node<StringType>(text));
}

template <typename StringType>
inline std::unique_ptr<basic_node> make_unique_prose (StringType && text)
{
    return std::unique_ptr<basic_node>(new prose_node<StringType>(std::forward<StringType>(text)));
}

////////////////////////////////////////////////////////////////////////////////
// rule_spec
////////////////////////////////////////////////////////////////////////////////
template <typename StringType>
class rule_spec final
{
public:
    using string_type = StringType;

private:
    string_type _name;

public:
    rule_spec () = delete;
    ~rule_spec () = default;

    rule_spec (rule_spec const &) = delete;
    rule_spec & operator = (rule_spec const &) = delete;

    rule_spec (rule_spec &&) = default;
    rule_spec & operator = (rule_spec &&) = default;
};

////////////////////////////////////////////////////////////////////////////////
// syntax_tree
////////////////////////////////////////////////////////////////////////////////

template <typename StringType>
class syntax_tree final
{
    using rule_spec_type = rule_spec<StringType>;

    std::vector<rule_spec_type> _rules;

public:
    syntax_tree () = default;
    ~syntax_tree () = default;

    syntax_tree (syntax_tree const & other) = delete;
    syntax_tree & operator = (syntax_tree const & other) = delete;

    syntax_tree (syntax_tree && other) = default;
    syntax_tree & operator = (syntax_tree && other) = default;
};

}}} // pfs::parser::abnf
