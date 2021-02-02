////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [pfs-parser](https://github.com/semenovf/pfs-parser) library.
//
// Changelog:
//      2021.02.02 Initial version
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <utility>

namespace pfs {
namespace parser {

/**
 * @brief Advance by repetition.
 *
 * @param pos On input - first position, on output - last good position.
 * @param last End of sequence position.
 * @param range Repetition range (including upper bound).
 * @param op Operation satisfying signature
 *        bool () (ForwardIterator &, ForwardIterator)
 * @return @c true if advanced by at least one position, otherwise @c false.
 */
template <typename ForwardIterator, typename RepetitionOp>
bool advance_repetition (ForwardIterator & pos
    , ForwardIterator last
    , std::pair<int, int> const & range
    , RepetitionOp op)
{
    bool success = true;
    decltype(range.first) lower_bound {0};

    for (int i = 0; pos != last && i < range.first; ++i, ++lower_bound) {
        if (! op(pos, last))
            break;
    }

    if (lower_bound != range.first)
        return false;

    decltype(range.second) upper_bound {0};

    for (int i = 0; pos != last && i < range.second; ++i, ++upper_bound) {
        if (! op(pos, last))
            break;
    }

    return true;
}

}} // namespace
