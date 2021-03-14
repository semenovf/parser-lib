////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [pfs-parser](https://github.com/semenovf/pfs-parser) library.
//
// Changelog:
//      2021.02.02 Initial version
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <limits>
#include <utility>
#include <cassert>

namespace pfs {
namespace parser {

using repeat_range = std::pair<int, int>;

inline repeat_range make_range (
      repeat_range::first_type lower_bound = repeat_range::first_type{0}
    , repeat_range::second_type upper_bound = repeat_range::first_type{
            std::numeric_limits<repeat_range::second_type>::max()})
{
    return std::make_pair(lower_bound, upper_bound);
}

inline repeat_range unlimited_range ()
{
    return make_range();
}

// Convert sequence '1*DIGIT' to integer value.
// Returns on error:
//      {0, false} if non-digit character found;
//      {std::numeric_limits<long>::max(), false} if overflow occured.
template <typename ForwardIterator>
std::pair<long, bool> to_decimal_number (ForwardIterator first, ForwardIterator last)
{
    using char_type = typename std::remove_reference<decltype(*first)>::type;
    constexpr int radix = 10;
    long result = 0;
    long cutoff_value = std::numeric_limits<long>::min() / radix;
    long cutoff_limit = std::numeric_limits<long>::min() % radix;

    cutoff_value *= -1;
    cutoff_limit *= -1;

    for (; first != last; ++first) {
        auto digit = uint32_t(*first) - uint32_t(char_type('0'));

        if (digit < 0 || digit > 9) {
            // Character is not a digit
            return std::make_pair(long{0}, false);
        }

        if (result < cutoff_value
                || (result == cutoff_value && static_cast<uintmax_t>(digit) <= cutoff_limit)) {
            result *= radix;
            result += static_cast<long>(digit);
        } else {
            // Too big, overflow
            return std::make_pair(std::numeric_limits<long>::max(), false);
        }
    }

    return std::make_pair(result, true);
}

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
bool advance_repetition_by_range (ForwardIterator & pos
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

}} // namespace pfs::parser
