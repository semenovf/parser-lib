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
    size_t _offset = 0;
    ForwardIterator _it;
    bool _is_CR = false;

public:
    line_counter_iterator (ForwardIterator initial)
        : _it(initial)
    {}

    line_counter_iterator (line_counter_iterator const & rhs)
        : _lineno(rhs._lineno)
        , _it(rhs._it)
        , _offset(rhs._offset)
    {}

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
        ++_offset;

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
        // It is not enough compare two base iterators, because, e.g.
        // two std::istream_iterator are equal if both of them are end-of-stream
        // iterators or both of them refer to the same stream.
        // See https://en.cppreference.com/w/cpp/iterator/istream_iterator/operator_cmp
        return _it == other._it && _offset == other._offset;
    }

    bool operator != (line_counter_iterator const & other)
    {
        return ! this->operator == (other);
    }
};

}} // namespace parser
