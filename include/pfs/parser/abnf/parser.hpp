////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [pfs-parser](https://github.com/semenovf/pfs-parser) library.
//
// Changelog:
//      2021.01.16 Initial version
////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "error.hpp"
#include "pfs/parser/core_rules.hpp"
#include <bitset>
// #include <functional>
// #include <iterator>
// #include <memory>
// #include <string>
#include <type_traits>
// #include <utility>
// #include <cassert>
// #include <clocale>
// #include <cstdlib>

namespace pfs {
namespace parser {
namespace abnf {

/**
 * [RFC 5234 - Augmented BNF for Syntax Specifications: ABNF] (https://tools.ietf.org/html/rfc5234)
 * [RFC 7405 - Case-Sensitive String Support in ABNF](https://tools.ietf.org/html/rfc7405)
 *
 * History:
 *
 * RFC 5234 obsoletes RFC 4234
 * RFC 4234 obsoletes RFC 2234
 */

enum parse_policy_flag {
    // Allow case sensitive sequence for rule name (default is case insensitive) as quotation mark besides double quote
      allow_case_sensitive_rulenames


//     , allow_array_root_element
//     , allow_number_root_element
//     , allow_string_root_element
//     , allow_boolean_root_element
//     , allow_null_root_element
//
//     // Allow apostrophe as quotation mark besides double quote
//     , allow_single_quote_mark
//
//     // Allow any escaped character in string not only permitted by grammar
//     , allow_any_char_escaped
//
//     // Clarification:
//     // Compare original 'number' grammar with modified:
//     // Original:
//     //      number = [ minus ] int ...
//     // Modified:
//     //      number = [ minus / plus] int ...
//     , allow_positive_signed_number

    , parse_policy_count
};

using parse_policy_set = std::bitset<parse_policy_count>;

// template <typename InputIt1, typename InputIt2>
// inline bool starts_with (InputIt1 first1, InputIt1 last1
//     , InputIt2 first2, InputIt1 last2)
// {
//     for (; first1 != last1 && first2 != last2; ++first1, ++first2) {
//         if (!(*first1 == *first2)) {
//             return false;
//         }
//     }
//
//     return first2 == last2;
// }
//
// template <typename CharType>
// inline bool starts_with (std::initializer_list<CharType> const & l1
//     , std::initializer_list<CharType> const & l2)
// {
//     return starts_with(l1.begin(), l1.end(), l2.begin(), l2.end());
// }

/**
 * @return @c true if @a ch is any 7-bit US-ASCII character, excluding NUL,
 *         otherwise @c false.
 *
 * @note Grammar
 * prose_value_char = %x20-3D / %x3F-7E
 */
template <typename CharType>
inline bool is_prose_value_char (CharType ch)
{
    auto a = uint32_t(ch) >= uint32_t(CharType('\x20'))
        && uint32_t(ch) <= uint32_t(CharType('\x3D'));

    return a || (uint32_t(ch) >= uint32_t(CharType('\x3F'))
        && uint32_t(ch) <= uint32_t(CharType('\x7E')));
}

/**
 * @brief Advance by @c prose value.
 *
 * @param pos On input - first position, on output - last good position.
 * @param last End of sequence position.
 * @return @c true if advanced by at least one position, otherwise @c false.
 *
 * @note Grammar
 * prose-val = "<" *(%x20-3D / %x3F-7E) ">"
 *             ; bracketed string of SP and VCHAR without angles
 *             ; prose description, to be used as last resort;
 *             ; %x3E - is a right bracket character '>'
 */
template <typename ForwardIterator>
bool advance_prose_value (ForwardIterator & pos
    , ForwardIterator last)
{
    using char_type = typename std::remove_reference<decltype(*pos)>::type;

    auto p = pos;

    if (p == last)
        return false;

    if (*p != char_type('<'))
        return false;

    ++p;

    while (p != last && is_prose_value_char(*p))
        ++p;

    if (p == last)
        return false;

    if (*p != char_type('>'))
        return false;

    ++p;

    return compare_and_assign(pos, p);
}

enum class number_flag {
    unspecified, binary, decimal, hexadecimal
};

/**
 * @brief Advance by @c number value.
 *
 * @param pos On input - first position, on output - last good position.
 * @param last End of sequence position.
 * @param callbacks Structure satisfying requirements of NumValueContext
 * @return @c true if advanced by at least one position, otherwise @c false.
 *
 * @par
 * NumValueContext {
 *     void first_number_value (number_flag, ForwardIterator first, ForwardIterator last)
 *     void last_number_value (number_flag, ForwardIterator first, ForwardIterator last)
 *     void next_number_value (number_flag, ForwardIterator first, ForwardIterator last)
 * }
 *
 * @note Grammar
 * num-val = "%" (bin-val / dec-val / hex-val)
 * bin-val = "b" 1*BIT [ 1*("." 1*BIT) / ("-" 1*BIT) ] ; series of concatenated bit values or single ONEOF range
 * dec-val = "d" 1*DIGIT [ 1*("." 1*DIGIT) / ("-" 1*DIGIT) ]
 * hex-val = "x" 1*HEXDIG [ 1*("." 1*HEXDIG) / ("-" 1*HEXDIG) ]
 */
template <typename ForwardIterator, typename NumValueContext>
bool advance_number_value (ForwardIterator & pos
    , ForwardIterator last
    , NumValueContext * ctx = nullptr)
{
    using char_type = typename std::remove_reference<decltype(*pos)>::type;

    auto p = pos;

    if (p == last)
        return false;

    if (*p != char_type('%'))
        return false;

    ++p;

    if (p == last)
        return false;

    number_flag flag = number_flag::unspecified;
    bool (* advance_func)(ForwardIterator &, ForwardIterator) = nullptr;
    bool (* is_digit_func)(char_type) = nullptr;

    if (*p == char_type('x')) {
        advance_func = advance_hexdigit_chars;
        is_digit_func = is_hexdigit_char;
        flag = number_flag::hexadecimal;
    } else if (*p == char_type('d')) {
        advance_func = advance_digit_chars;
        is_digit_func = is_digit_char;
        flag = number_flag::decimal;
    } else if (*p == char_type('b')) {
        advance_func = advance_bit_chars;
        is_digit_func = is_bit_char;
        flag = number_flag::binary;
    } else {
        return false;
    }

    ++p;

    if (p == last)
        return false;

    auto first_pos = p;

    if (!advance_func(p, last))
        return false;

    if (ctx)
        ctx->first_number_value(flag, first_pos, p);

    if (p != last) {
        if (*p == char_type('-')) {
            ++p;

            // At least one digit character must exists
            if (!is_digit_func(*p))
                return false;

            first_pos = p;

            if (!advance_func(p, last))
                return false;

            if (ctx)
                ctx->last_number_value(flag, first_pos, p);
        } else if (*p == char_type('.')) {
            while (*p == char_type('.')) {
                ++p;

                // At least one digit character must exists
                if (!is_digit_func(*p))
                    return false;

                first_pos = p;

                if (!advance_func(p, last))
                    return false;

                if (ctx)
                    ctx->next_number_value(flag, first_pos, p);
            }

            // Notify observer no more valid elements parsed
            if (ctx)
                ctx->last_number_value(flag, p, p);
        }
    }

    return compare_and_assign(pos, p);
}

/**
 * @brief Advance by quoted string.
 *
 * @param pos On input - first position, on output - last good position.
 * @param last End of sequence position.
 * @param ctx Structure satisfying requirements of QuotedStringContext
 * @error @c unbalanced_quote if quote is unbalanced.
 * @error @c bad_quoted_char if quoted string contains invalid character.
 * @error @c max_lengh_exceeded if quoted string length too long.
 * @return @c true if advanced by at least one position, otherwise @c false.
 *
 * @par
 * QuotedStringContext {
 *     size_t max_quoted_string_length ()
 *     void quoted_string (ForwardIterator first, ForwardIterator last)
 *     void error (error_code ec);
 * }
 *
 * @note Grammar
 * char-val = DQUOTE *(%x20-21 / %x23-7E) DQUOTE
 *                  ; quoted string of SP and VCHAR
 *                  ; without DQUOTE (%x22)
 */
template <typename ForwardIterator, typename QuotedStringContext>
bool advance_quoted_string (ForwardIterator & pos
    , ForwardIterator last
    , QuotedStringContext * ctx = nullptr)
{
    using char_type = typename std::remove_reference<decltype(*pos)>::type;

    auto p = pos;

    if (p == last)
        return false;

    if (!is_dquote_char(*p))
        return false;

    ++p;

    auto first_pos = p;

    if (p == last) {
        if (ctx) {
            auto ec = make_error_code(errc::unbalanced_quote);
            ctx->error(ec);
        }
        return false;
    }

    size_t length = 0;
    size_t max_length = ctx
        ? ctx->max_quoted_string_length()
        : std::numeric_limits<size_t>::max();

    if (max_length == 0)
        max_length = std::numeric_limits<size_t>::max();

    // Parse quoted string of SP and VCHAR without DQUOTE
    while (p != last && !is_dquote_char(*p)) {
        if (!(is_visible_char(*p) || is_space_char(*p))) {
            if (ctx) {
                auto ec = make_error_code(errc::bad_quoted_char);
                ctx->error(ec);
            }

            return false;
        }

        if (length == max_length) {
            if (ctx) {
                auto ec = make_error_code(errc::max_lengh_exceeded);
                ctx->error(ec);
            }
            return false;
        }

        ++length;
        ++p;
    }

    if (p == last) {
        if (ctx) {
            auto ec = make_error_code(errc::unbalanced_quote);
            ctx->error(ec);
        }
        return false;
    }

    if (ctx) {
        ctx->quoted_string(first_pos, p);
    }

    ++p; // Skip DQUOTE

    return compare_and_assign(pos, p);
}

/**
 * @brief Advance by 'repeat' rule.
 *
 * @param pos On input - first position, on output - last good position.
 * @param last End of sequence position.
 * @param ctx Structure satisfying requirements of RepeatContext
 * @return @c true if advanced by at least one position, otherwise @c false.
 *
 * @par
 * RepeatContext {
 *     void repeat (ForwardIterator first_from, ForwardIterator last_from
 *          , ForwardIterator first_to, ForwardIterator last_to)
 * }
 *
 * if first_from == last_from -> no low limit (*1)
 * if first_to == toLast -> no upper limit (1*)
 * if first_from == first_to && first_from != last_from -> is exact limit
 *
 * @note Grammar
 * repeat = 1*DIGIT / (*DIGIT "*" *DIGIT)
 */
template <typename ForwardIterator, typename RepeatContext>
bool advance_repeat (ForwardIterator & pos
    , ForwardIterator last
    , RepeatContext * ctx = nullptr)
{
    using char_type = typename std::remove_reference<decltype(*pos)>::type;

    auto p = pos;

    if (p == last)
        return false;

    ForwardIterator first_from = pos;
    ForwardIterator last_from  = pos;
    ForwardIterator first_to   = pos;
    ForwardIterator last_to    = pos;

    do {
        if (is_digit_char(*p)) {
            advance_digit_chars(p, last);
            last_from = p;

            // Success, is exact limit
            if (p == last) {
                first_to = first_from;
                last_to = last_from;
                break;
            }
        }

        if (*p == char_type('*')) {
            ++p;

            if (p == last) { // Success, second part is empty
                break;
            } else if (is_digit_char(*p)) {
                first_to = p;
                advance_digit_chars(p, last);
                last_to = p;
                break;
            } else { // Success, second part is empty
                break;
            }
        }
    } while (false);

    if (p != pos && ctx)
        ctx->repeat(first_from, last_from, first_to, last_to);

    return compare_and_assign(pos, p);
}

/**
 * @brief Advance by comment.
 *
 * @param pos On input - first position, on output - last good position.
 * @param last End of sequence position.
 * @param ctx Structure satisfying requirements of CommentContext
 * @return @c true if advanced by at least one position, otherwise @c false.
 *
 * @par
 * CommentContext {
 *     void comment (ForwardIterator first, ForwardIterator last)
 * }
 *
 * @note Grammar
 * comment = ";" *(< neither CR nor LF character >) CRLF
 *
 * This grammar replaces more strict one from RFC 5234:
 * comment = ";" *(WSP / VCHAR) CRLF
 */
template <typename ForwardIterator, typename CommentContext>
bool advance_comment (ForwardIterator & pos
    , ForwardIterator last
    , CommentContext * ctx = nullptr)
{
    using char_type = typename std::remove_reference<decltype(*pos)>::type;

    auto p = pos;

    if (p == last)
        return false;

    if (*p != char_type(';'))
        return false;

    ++p;

    ForwardIterator first_pos = p;

    while (p != last && !(is_cr_char(*p) || is_lf_char(*p))) {
        ++p;
    }

    if (ctx)
        ctx->comment(first_pos, p);

    if (p != last)
        advance_newline(p, last);

    return compare_and_assign(pos, p);
}

/**
 * @brief Advance by comment or new line.
 *
 * @param pos On input - first position, on output - last good position.
 * @param last End of sequence position.
 * @param ctx Structure satisfying requirements of CommentContext
 * @return @c true if advanced by at least one position, otherwise @c false.
 *
 * @par
 * CommentContext {
 *     void comment (ForwardIterator first, ForwardIterator last)
 * }
 *
 * @note Grammar
 * c-nl = comment / CRLF ; comment or newline
 */
template <typename ForwardIterator, typename CommentContext>
inline bool advance_comment_newline (ForwardIterator & pos
    , ForwardIterator last
    , CommentContext * ctx = nullptr)
{
    return advance_newline(pos, last)
        || advance_comment(pos, last, ctx);
    using char_type = typename std::remove_reference<decltype(*pos)>::type;
}

/**
 * @brief Advance by rule name.
 *
 * @param pos On input - first position, on output - last good position.
 * @param last End of sequence position.
 * @param ctx Structure satisfying requirements of RulenameContext
 * @return @c true if advanced by at least one position, otherwise @c false.
 *
 * @par
 * RulenameContext {
 *     void rulename (ForwardIterator first, ForwardIterator last)
 * }
 *
 * @note Grammar
 * rulename = ALPHA *(ALPHA / DIGIT / "-")
 */
template <typename ForwardIterator, typename RulenameContext>
bool advance_rulename (ForwardIterator & pos
    , ForwardIterator last
    , RulenameContext * ctx = nullptr)
{
    using char_type = typename std::remove_reference<decltype(*pos)>::type;

    auto p = pos;

    if (p == last)
        return false;

    if (!is_alpha_char(*p))
        return false;

    ForwardIterator first_pos = p;

    ++p;

    while (p != last
        && (is_alpha_char(*p)
            || is_digit_char(*p)
            || *p == char_type('-'))) {
        ++p;
    }

    if (ctx)
        ctx->rulename(first_pos, p);

    return compare_and_assign(pos, p);
}

// /**
//  * @brief Advance by whitespace or comment or new line and whitespace.
//  *
//  * @param pos On input - first position, on output - last good position.
//  * @param last End of sequence position.
//  * @param ctx Structure satisfying requirements of CommentContext
//  * @return @c true if advanced by at least one position, otherwise @c false.
//  *
//  * @par
//  * CommentContext {
//  *     void comment (ForwardIterator first, ForwardIterator last)
//  * }
//  *
//  * @note Grammar
//  * c-wsp = WSP / (c-nl WSP)
//  */
// template <typename ForwardIterator, typename CommentContext>
// inline bool advance_comment_newline_whitespace (ForwardIterator & pos
//     , ForwardIterator last
//     , CommentContext * ctx = nullptr)
// {
//     if (p == last)
//         return false;
//
//     if (is_whitespace_char(*p))
//         ++p;
//     else
//
//     return advance_newline(pos, last)
//         || advance_comment(pos, last, ctx);
//     using char_type = typename std::remove_reference<decltype(*pos)>::type;
// }

// TODO
// * rulelist
// * rule
// * defined-as
// * elements
// * c-wsp
// * alternation
// * concatenation
// * repetition
// * element
// * group
// * option

////////////////////////////////////////////////////////////////////////////////
// advance_rule
////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Advance by @c rule sequence.
 *
 * @param pos On input - first position, on output - last good position.
 * @param last End of sequence position.
 * @return @c true if advanced by at least one position, otherwise @c false.
 *
 * @note Rule names are case insensitive.
 * @note Grammar
 * name =  elements crlf
 */
template <typename _ForwardIterator>
bool advance_rule (_ForwardIterator & pos
        , _ForwardIterator last)
{
    auto p = pos;

    // FIXME
//     while (p != last && is_whitespace(*p))
//         ++p;

    return compare_and_assign(pos, p);
}

}}} // // namespace pfs::parser::abnf
