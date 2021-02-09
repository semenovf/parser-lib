////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [pfs-parser](https://github.com/semenovf/pfs-parser) library.
//
// Changelog:
//      2021.01.17 Initial version
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <type_traits>

namespace pfs {
namespace parser {

template <typename ForwardIterator>
struct line_counter_iterator
{
    size_t _lineno = 1;
    ForwardIterator _it;
    bool _is_CR = false;

public:
    line_counter_iterator (ForwardIterator initial)
        : _it(initial)
    {}

    line_counter_iterator (line_counter_iterator const & rhs)
        : _lineno(rhs._lineno)
        , _it(rhs._it)
    {}

    ForwardIterator base ()
    {
        return _it;
    }

    auto operator * () const -> decltype(*_it)
    {
        return *_it;
    }

    line_counter_iterator & operator ++ ()
    {
        using char_type = typename std::remove_reference<decltype(*_it)>::type;

        if (*_it == '\x0D')
            _is_CR == true;
        else
            _is_CR = false;

        ++_it;

        if (*_it == '\x0D' || (*_it == '\x0A' && !_is_CR))
            ++_lineno;

        return *this;
    }

    line_counter_iterator operator ++ (int)
    {
        line_counter_iterator it(*this);
        ++(*this);
        return it;
    }

    size_t lineno () const
    {
        return _lineno;
    }

    bool operator == (line_counter_iterator const & other)
    {
        return _it == other._it;
    }

    bool operator != (line_counter_iterator const & other)
    {
        return ! this->operator == (other);
    }
};

}} // namespace parser
