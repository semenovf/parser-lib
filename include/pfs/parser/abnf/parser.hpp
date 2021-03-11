////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [pfs-parser](https://github.com/semenovf/pfs-parser) library.
//
// Changelog:
//      2021.01.16 Initial version.
//      2021.02.07 Alpha release.
//
// TODO 1. Add Concepts.
//      2. Implement Abstract Syntax Tree.
//
////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "error.hpp"
#include "pfs/parser/core_rules.hpp"
#include "pfs/parser/generator.hpp"
#include <bitset>
#include <type_traits>

#if __cplusplus > 201703L && __cpp_concepts >= 201907L
#   include <concepts>
#endif

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
    // Allow case sensitive sequence for rule name (default is case insensitive)
    // as quotation mark besides double quote
    // TODO Is not applicable yet
      allow_case_sensitive_rulenames

    , parse_policy_count
};

using parse_policy_set = std::bitset<parse_policy_count>;

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
 * @param ctx Structure satisfying requirements of ProseContext.
 * @return @c true if advanced by at least one position, otherwise @c false.
 *
 * @par
 * ProseContext {
 *     void prose (ForwardIterator first, ForwardIterator last)
 * }
 *
 * @note Grammar
 * prose-val = "<" *(%x20-3D / %x3F-7E) ">"
 *             ; bracketed string of SP and VCHAR without angles
 *             ; prose description, to be used as last resort;
 *             ; %x3E - is a right bracket character '>'
 */
template <typename ForwardIterator, typename ProseContext>
bool advance_prose (ForwardIterator & pos
    , ForwardIterator last
    , ProseContext * ctx = nullptr)
{
    using char_type = typename std::remove_reference<decltype(*pos)>::type;

    auto p = pos;

    if (p == last)
        return false;

    if (*p != char_type('<'))
        return false;

    ++p;

    auto first_pos = p;

    while (p != last && is_prose_value_char(*p))
        ++p;

    if (p == last)
        return false;

    if (*p != char_type('>'))
        return false;

    if (ctx)
        ctx->prose(first_pos, p);

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
 * @param ctx Structure satisfying requirements of NumberContext
 * @return @c true if advanced by at least one position, otherwise @c false.
 *
 * @par
 * NumberContext {
 *     void first_number (number_flag, ForwardIterator first, ForwardIterator last)
 *     void last_number (number_flag, ForwardIterator first, ForwardIterator last)
 *     void next_number (number_flag, ForwardIterator first, ForwardIterator last)
 * }
 *
 * @note Grammar
 * num-val = "%" (bin-val / dec-val / hex-val)
 * bin-val = "b" 1*BIT [ 1*("." 1*BIT) / ("-" 1*BIT) ] ; series of concatenated bit values or single ONEOF range
 * dec-val = "d" 1*DIGIT [ 1*("." 1*DIGIT) / ("-" 1*DIGIT) ]
 * hex-val = "x" 1*HEXDIG [ 1*("." 1*HEXDIG) / ("-" 1*HEXDIG) ]
 */
template <typename ForwardIterator, typename NumberContext>
bool advance_number (ForwardIterator & pos
    , ForwardIterator last
    , NumberContext * ctx = nullptr)
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
        ctx->first_number(flag, first_pos, p);

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
                ctx->last_number(flag, first_pos, p);
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
                    ctx->next_number(flag, first_pos, p);
            }

            // Notify observer no more valid elements parsed
            if (ctx)
                ctx->last_number(flag, p, p);
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
}

/**
 * @brief Advance by white space or group of comment or new line with white space.
 *
 * @param pos On input - first position, on output - last good position.
 * @param last End of sequence position.
 * @param ctx Structure satisfying requirements of CommentContext
 * @return @c true if advanced by at least one position, otherwise @c false.
 *
 * @note Grammar
 * c-wsp =  WSP / (c-nl WSP)
 */
template <typename ForwardIterator, typename CommentContext>
inline bool advance_comment_whitespace (ForwardIterator & pos
    , ForwardIterator last
    , CommentContext * ctx = nullptr)
{
    using char_type = typename std::remove_reference<decltype(*pos)>::type;

    if (pos == last)
        return false;

    auto p = pos;

    if (is_whitespace_char(*p)) {
        ++p;
    } else if (advance_comment_newline(p, last, ctx)) {
        if (p == last)
            return false;

        if (! is_whitespace_char(*p))
            return false;

        ++p;
    } else {
        return false;
    }

    return compare_and_assign(pos, p);
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

    if (pos == last)
        return false;

    auto p = pos;

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

// Forward decalration for advance_group() and advance_option()
// used in advance_element()

template <typename ForwardIterator, typename GroupContext>
bool advance_group (ForwardIterator &
    , ForwardIterator
    , GroupContext * = nullptr);

template <typename ForwardIterator, typename OptionContext>
bool advance_option (ForwardIterator &
    , ForwardIterator
    , OptionContext * = nullptr);

/**
 * @brief Advance by element.
 *
 * @param pos On input - first position, on output - last good position.
 * @param last End of sequence position.
 * @param ctx Structure satisfying requirements of ElementContext
 * @return @c true if advanced by at least one position, otherwise @c false.
 *
 * @par
 * ElementContext extends RulenameContext
 *      , GroupContext
 *      , OptionContext
 *      , QuotedStringContext
 *      , NumberContext
 *      , ProseContext { }
 *
 * @note Grammar
 * element = rulename / group / option / char-val / num-val / prose-val
 */
template <typename ForwardIterator, typename ElementContext>
bool advance_element (ForwardIterator & pos
    , ForwardIterator last
    , ElementContext * ctx = nullptr)
{
    if (pos == last)
        return false;

    return advance_rulename(pos, last, ctx)
        || advance_group(pos, last, ctx)
        || advance_option(pos, last, ctx)
        || advance_number(pos, last, ctx)
        || advance_quoted_string(pos, last, ctx)
        || advance_prose(pos, last, ctx);
}

/**
 * @brief Advance by repetition.
 *
 * @param pos On input - first position, on output - last good position.
 * @param last End of sequence position.
 * @param ctx Structure satisfying requirements of RepetitionContext
 * @return @c true if advanced by at least one position, otherwise @c false.
 *
 * @par
 * RepetitionContext extends RepeatContext
 *      , ElementContext
 * {
 *      void begin_repetition ()
 *      void end_repetition (bool success)
 * }
 *
 * @note Grammar
 * repetition = [repeat] element
 */
template <typename ForwardIterator, typename RepetitionContext>
bool advance_repetition (ForwardIterator & pos
    , ForwardIterator last
    , RepetitionContext * ctx = nullptr)
{
    if (pos == last)
        return false;

    if (ctx)
        ctx->begin_repetition();

    advance_repeat(pos, last, ctx);

    auto success = advance_element(pos, last, ctx);

    if (ctx)
        ctx->end_repetition(success);

    return success;
}

/**
 * @brief Advance by concatenation.
 *
 * @param pos On input - first position, on output - last good position.
 * @param last End of sequence position.
 * @param ctx Structure satisfying requirements of ConcatenationContext
 * @return @c true if advanced by at least one position, otherwise @c false.
 *
 * @par
 * ConcatenationContext extends RepetitionContext
 *      , CommentContext
 * {
 *      void begin_concatenation ()
 *      void end_concatenation (bool success)
 * }
 *
 * @note Grammar
 * concatenation = repetition *(1*c-wsp repetition)
 */
template <typename ForwardIterator, typename ConcatenationContext>
bool advance_concatenation (ForwardIterator & pos
    , ForwardIterator last
    , ConcatenationContext * ctx = nullptr)
{
    if (pos == last)
        return false;

    if (ctx)
        ctx->begin_concatenation();

    // At least one repetition need
    if (!advance_repetition(pos, last, ctx))
        return false;

    // *(1*c-wsp repetition)
    return advance_repetition_by_range(pos, last, unlimited_range()
        , [ctx] (ForwardIterator & pos, ForwardIterator last) -> bool {

            auto p = pos;

            // 1*c-wsp
            if (! advance_repetition_by_range(p, last, make_range(1)
                    , [ctx] (ForwardIterator & pos, ForwardIterator last) -> bool {
                        return advance_comment_whitespace(pos, last, ctx);
                    })) {
                return false;
            }

            if (! advance_repetition(p, last, ctx))
                return false;

            return compare_and_assign(pos, p);
        });
}

/**
 * @brief Advance by alternation.
 *
 * @param pos On input - first position, on output - last good position.
 * @param last End of sequence position.
 * @param ctx Structure satisfying requirements of AlternationContext
 * @return @c true if advanced by at least one position, otherwise @c false.
 *
 * @par
 * AlternationContext extends ConcatenationContext
 * { }
 *
 * @note Grammar
 * alternation = concatenation *(*c-wsp "/" *c-wsp concatenation)
 */
template <typename ForwardIterator, typename AlternationContext>
bool advance_alternation (ForwardIterator & pos
    , ForwardIterator last
    , AlternationContext * ctx = nullptr)
{
    using char_type = typename std::remove_reference<decltype(*pos)>::type;

    if (pos == last)
        return false;

    if (! advance_concatenation(pos, last, ctx))
        return false;

    // _________________________________________________
    // |                                               |
    // *(*c-wsp "/" *c-wsp concatenation)              v
    return advance_repetition_by_range(pos, last, unlimited_range()
        , [ctx] (ForwardIterator & pos, ForwardIterator last) -> bool {
            auto p = pos;

            // *c-wsp
            while (advance_comment_whitespace(p, last, ctx))
                ;

            if (p == last)
                return false;

            if (*p != char_type('/'))
                return false;

            ++p;

            if (p == last)
                return false;

            // *c-wsp
            while (advance_comment_whitespace(p, last, ctx))
                ;

            if (! advance_concatenation(p, last, ctx))
                return false;

            return compare_and_assign(pos, p);
        });
}

/* Helper function
 *
 * Grammar
 * group  = "(" *c-wsp alternation *c-wsp ")"
 * option = "[" *c-wsp alternation *c-wsp "]"
 */
template <typename ForwardIterator, typename GroupContext>
bool advance_group_or_option (ForwardIterator & pos
    , ForwardIterator last
    , bool is_group
    , GroupContext * ctx = nullptr)
{
    using char_type = typename std::remove_reference<decltype(*pos)>::type;

    char_type opening_bracket = is_group ? char_type('(') : char_type('[');
    char_type closing_bracket = is_group ? char_type(')') : char_type(']');

    if (pos == last)
        return false;

    auto p = pos;

    if (*p != opening_bracket)
        return false;

    ++p;

    // *c-wsp
    while (advance_comment_whitespace(p, last, ctx))
        ;

    if (p == last)
        return false;

    if (! advance_alternation(p, last, ctx))
        return false;

    // *c-wsp
    while (advance_comment_whitespace(p, last, ctx))
        ;

    if (*p != closing_bracket)
        return false;

    ++p;

    return compare_and_assign(pos, p);
}

/**
 * @brief Advance by group.
 *
 * @param pos On input - first position, on output - last good position.
 * @param last End of sequence position.
 * @param ctx Structure satisfying requirements of GroupContext
 * @return @c true if advanced by at least one position, otherwise @c false.
 *
 * @par
 * GroupContext extends AlternationContext
 * { }
 *
 * @note Grammar
 * group  = "(" *c-wsp alternation *c-wsp ")"
 */
template <typename ForwardIterator, typename GroupContext>
bool advance_group (ForwardIterator & pos
    , ForwardIterator last
    , GroupContext * ctx)
{
    return advance_group_or_option(pos, last, true, ctx);
}

/**
 * @brief Advance by option.
 *
 * @param pos On input - first position, on output - last good position.
 * @param last End of sequence position.
 * @param ctx Structure satisfying requirements of OptionContext
 * @return @c true if advanced by at least one position, otherwise @c false.
 *
 * @par
 * OptionContext extends AlternationContext
 * { }
 *
 * @note Grammar
 * option = "[" *c-wsp alternation *c-wsp "]"
 */
template <typename ForwardIterator, typename OptionContext>
bool advance_option (ForwardIterator & pos
    , ForwardIterator last
    , OptionContext * ctx)
{
    return advance_group_or_option(pos, last, false, ctx);
}

/**
 * @brief Advance by @c defined-as.
 *
 * @param pos On input - first position, on output - last good position.
 * @param last End of sequence position.
 * @param ctx Structure satisfying requirements of DefinedAsContext
 * @return @c true if advanced by at least one position, otherwise @c false.
 *
 * @par
 * DefinedAsContext extends CommentContext
 * {
 *      void accept_basic_rule_definition ();
 *      void accept_incremental_alternatives ();
 * }
 *
 * @note Grammar
 * defined-as = *c-wsp ("=" / "=/") *c-wsp
 *          ; basic rules definition and
 *          ; incremental alternatives
 */
template <typename ForwardIterator, typename DefinedAsContext>
bool advance_defined_as (ForwardIterator & pos
    , ForwardIterator last
    , DefinedAsContext * ctx = nullptr)
{
    using char_type = typename std::remove_reference<decltype(*pos)>::type;

    if (pos == last)
        return false;

    bool is_basic_rules_definition = true;
    auto p = pos;

    // *c-wsp
    while (advance_comment_whitespace(p, last, ctx))
        ;

    if (p == last)
        return false;

    if (*p == char_type('=')) {
        ++p;

        if (p != last) {
            if (*p == char_type('/')) {
                ++p;
                is_basic_rules_definition = false;
            }
        }
    } else {
        return false;
    }

    // *c-wsp
    while (advance_comment_whitespace(p, last, ctx))
        ;

    if (ctx) {
        if (is_basic_rules_definition)
            ctx->accept_basic_rule_definition();
        else
            ctx->accept_incremental_alternatives();
    }

    return compare_and_assign(pos, p);
}

/**
 * @brief Advance by @c elements.
 *
 * @param pos On input - first position, on output - last good position.
 * @param last End of sequence position.
 * @param ctx Structure satisfying requirements of ElementsContext
 * @return @c true if advanced by at least one position, otherwise @c false.
 *
  * @par
 * ElementsContext extends AlternationContext
 * {}
 *
 * @note Grammar
 * elements = alternation *c-wsp
 */
template <typename ForwardIterator, typename ElementsContext>
bool advance_elements (ForwardIterator & pos
    , ForwardIterator last
    , ElementsContext * ctx = nullptr)
{
    auto p = pos;

    if (!advance_alternation(p, last, ctx))
        return false;

    // *c-wsp
    while (advance_comment_whitespace(p, last, ctx))
        ;

    return compare_and_assign(pos, p);
}

/**
 * @brief Advance by @c rule.
 *
 * @param pos On input - first position, on output - last good position.
 * @param last End of sequence position.
 * @param ctx Structure satisfying requirements of RuleContext
 * @return @c true if advanced by at least one position, otherwise @c false.
 *
 * @par
 * RuleContext extends RulenameContext
 *      , ElementsContext
 *      , DefinedAsContext
 *      , CommentContext
 * {}
 *
 * @note Grammar
 * rule = rulename defined-as elements c-nl
 *          ; continues if next line starts
 *          ; with white space
 */
template <typename ForwardIterator, typename RuleContext>
bool advance_rule (ForwardIterator & pos
    , ForwardIterator last
    , RuleContext * ctx = nullptr)
{
    auto p = pos;

    if (!advance_rulename(p, last, ctx))
        return false;

    if (!advance_defined_as(p, last, ctx))
        return false;

    if (!advance_elements(p, last, ctx))
        return false;

    if (p != last) {
        if (!advance_comment_newline(p, last, ctx))
            return false;
    }

    return compare_and_assign(pos, p);
}

/**
 * @brief Advance by @c rulelist.
 *
 * @param pos On input - first position, on output - last good position.
 * @param last End of sequence position.
 * @param ctx Structure satisfying requirements of RulelistContext
 * @return @c true if advanced by at least one position, otherwise @c false.
 *
 * @par
 * RulelistContext extends RuleContext
 *      , CommentContext
 * {}
 *
 * @note Grammar
 * rulelist = 1*( rule / (*c-wsp c-nl) )
 */
template <typename ForwardIterator, typename RulelistContext>
bool advance_rulelist (ForwardIterator & pos
    , ForwardIterator last
    , RulelistContext * ctx = nullptr)
{
    return advance_repetition_by_range(pos, last, make_range(1)
        , [ctx] (ForwardIterator & pos, ForwardIterator last) -> bool {

            auto p = pos;

            if (advance_rule(p, last, ctx)) {
                ;
            } else {
                // *c-wsp
                while (advance_comment_whitespace(p, last, ctx))
                    ;

                if (p != last) {
                    if (!advance_comment_newline(p, last, ctx))
                        return false;
                }
            }

            return compare_and_assign(pos, p);
        });
}

}}} // // namespace pfs::parser::abnf
