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
 * @param callbacks Structure satisfying requirements of HexValueCallbacks
 * @return @c true if advanced by at least one position, otherwise @c false.
 *
 * @par
 * NumValueCallbacks {
 *     first_number_value (number_flag, ForwardIterator first, ForwardIterator last)
 *     last_number_value (number_flag, ForwardIterator first, ForwardIterator last)
 *     next_number_value (number_flag, ForwardIterator first, ForwardIterator last)
 * }
 *
 * @note Grammar
 * num-val = "%" (bin-val / dec-val / hex-val)
 * bin-val = "b" 1*BIT [ 1*("." 1*BIT) / ("-" 1*BIT) ] ; series of concatenated bit values or single ONEOF range
 * dec-val = "d" 1*DIGIT [ 1*("." 1*DIGIT) / ("-" 1*DIGIT) ]
 * hex-val = "x" 1*HEXDIG [ 1*("." 1*HEXDIG) / ("-" 1*HEXDIG) ]
 */
template <typename ForwardIterator, typename Callbacks>
bool advance_number_value (ForwardIterator & pos
    , ForwardIterator last
    , Callbacks * callbacks = nullptr)
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

    if (callbacks)
        callbacks->first_number_value(flag, first_pos, p);

    if (p != last) {
        if (*p == char_type('-')) {
            ++p;

            // At least one digit character must exists
            if (!is_digit_func(*p))
                return false;

            first_pos = p;

            if (!advance_func(p, last))
                return false;

            if (callbacks)
                callbacks->last_number_value(flag, first_pos, p);
        } else if (*p == char_type('.')) {
            while (*p == char_type('.')) {
                ++p;

                // At least one digit character must exists
                if (!is_digit_func(*p))
                    return false;

                first_pos = p;

                if (!advance_func(p, last))
                    return false;

                if (callbacks)
                    callbacks->next_number_value(flag, first_pos, p);
            }

            // Notify observer no more valid elements parsed
            if (callbacks)
                callbacks->last_number_value(flag, p, p);
        }
    }

    return compare_and_assign(pos, p);
}

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

#if __COMMENT__
template <typename T>
using range = std::pair<T,T>;

using std::begin;
using std::end;

////////////////////////////////////////////////////////////////////////////////
// basic_callbacks
////////////////////////////////////////////////////////////////////////////////
template <typename _StringType, typename _NumberType>
struct basic_callbacks
{
    using number_type = _NumberType;
    using string_type = _StringType;

    std::function<void(error_code const &)> on_error = [] (error_code const &) {};
    std::function<void()> on_null = [] {};
    std::function<void()> on_true = [] {};
    std::function<void()> on_false = [] {};
    std::function<void(number_type &&)> on_number = [] (number_type &&) {};
    std::function<void(string_type &&)> on_string = [] (string_type &&) {};
    std::function<void(string_type &&)> on_member_name = [] (string_type &&) {};
    std::function<void()> on_begin_array = [] {};
    std::function<void()> on_end_array = [] {};
    std::function<void()> on_begin_object = [] {};
    std::function<void()> on_end_object = [] {};
};



////////////////////////////////////////////////////////////////////////////////
// Policy satisfying RFC 4627
////////////////////////////////////////////////////////////////////////////////
inline parse_policy_set rfc4627_policy ()
{
    parse_policy_set result;
    result.set(allow_object_root_element, true);
    result.set(allow_array_root_element, true);
    return result;
}

////////////////////////////////////////////////////////////////////////////////
// Policy satisfying RFC 7159
////////////////////////////////////////////////////////////////////////////////
inline parse_policy_set rfc7159_policy ()
{
    parse_policy_set result;
    result.set(allow_object_root_element, true);
    result.set(allow_array_root_element, true);
    result.set(allow_number_root_element, true);
    result.set(allow_string_root_element, true);
    result.set(allow_boolean_root_element, true);
    result.set(allow_null_root_element, true);
    return result;
}

////////////////////////////////////////////////////////////////////////////////
// Policy satisfying JSON5
////////////////////////////////////////////////////////////////////////////////
inline parse_policy_set json5_policy ()
{
    parse_policy_set result = rfc7159_policy();
    result.set(allow_single_quote_mark, true);
    return result;
}

////////////////////////////////////////////////////////////////////////////////
// Strict policy
////////////////////////////////////////////////////////////////////////////////
inline parse_policy_set strict_policy ()
{
    return rfc7159_policy();
}

////////////////////////////////////////////////////////////////////////////////
// Relaxed policy
////////////////////////////////////////////////////////////////////////////////
inline parse_policy_set relaxed_policy ()
{
    parse_policy_set result = json5_policy();
    result.set(allow_positive_signed_number, true);
    result.set(allow_any_char_escaped, true);
    return result;
}

////////////////////////////////////////////////////////////////////////////////
// Default policy
////////////////////////////////////////////////////////////////////////////////
inline parse_policy_set default_policy ()
{
    return relaxed_policy();
}

////////////////////////////////////////////////////////////////////////////////
// locale_decimal_point
////////////////////////////////////////////////////////////////////////////////
/**
 * Get locale-specific decimal point character.
 */
inline char locale_decimal_point ()
{
    std::lconv const * loc = std::localeconv();
    assert(loc);
    return (loc->decimal_point && *loc->decimal_point != '\x0')
            ? *loc->decimal_point : '.';
}

////////////////////////////////////////////////////////////////////////////////
// Convert string to number functions
////////////////////////////////////////////////////////////////////////////////
inline bool strtointeger (long int & n, std::string const & numstr)
{
    char * endptr = nullptr;
    n = std::strtol(numstr.c_str(), & endptr, 10);
    return (errno != ERANGE && endptr == & *numstr.end());
}

inline bool strtointeger (long long int & n, std::string const & numstr)
{
    char * endptr = nullptr;
    n = std::strtoll(numstr.c_str(), & endptr, 10);
    return (errno != ERANGE && endptr == & *numstr.end());
}

inline bool strtouinteger (unsigned long int & n, std::string const & numstr)
{
    char * endptr = nullptr;
    n = std::strtoul(numstr.c_str(), & endptr, 10);
    return (errno != ERANGE && endptr == & *numstr.end());
}

inline bool strtouinteger (unsigned long long int & n, std::string const & numstr)
{
    char * endptr = nullptr;
    n = std::strtoull(numstr.c_str(), & endptr, 10);
    return (errno != ERANGE && endptr == & *numstr.end());
}

inline bool strtoreal (float & n, std::string const & numstr)
{
    char * endptr = nullptr;
    n = std::strtof(numstr.c_str(), & endptr);
    return (errno != ERANGE && endptr == & *numstr.end());
}

inline bool strtoreal (double & n, std::string const & numstr)
{
    char * endptr = nullptr;
    n = std::strtod(numstr.c_str(), & endptr);
    return (errno != ERANGE && endptr == & *numstr.end());
}


////////////////////////////////////////////////////////////////////////////////
// is_whitespace
////////////////////////////////////////////////////////////////////////////////
/**
 * @return @c true if character is one of the symbols:
 *      - space (%x20),
 *      - horizontal tab (%x09),
 *      - line feed or new line (%x0A),
 *      - carriage return (%x0D),
 *
 *      otherwise @c false.
 */
template <typename _CharT>
inline bool is_whitespace (_CharT ch)
{
    return (ch == _CharT('\x20')
            || ch == _CharT('\x09')
            || ch == _CharT('\x0A')
            || ch == _CharT('\x0D'));
}


////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
/**
 * @return Base-@a radix digit converted from character @a ch,
 *      or -1 if conversion is impossible. @a radix must be between 2 and 36
 *      inclusive.
 */
template <typename _CharT>
int to_digit (_CharT ch, int radix = 10)
{
    int digit = 0;

    // Bad radix
    if (radix < 2 || radix > 36)
        return -1;

    if (int(ch) >= int('0') && int(ch) <= int('9'))
        digit = int(ch) - int('0');
    else if (int(ch) >= int('a') && int(ch) <= int('z'))
        digit = int(ch) - int('a') + 10;
    else if (int(ch) >= int('A') && int(ch) <= int('Z'))
        digit = int(ch) - int('A') + 10;
    else
        return -1;

    if (digit >= radix)
        return -1;

    return digit;
}

////////////////////////////////////////////////////////////////////////////////
// is_quotation_mark
////////////////////////////////////////////////////////////////////////////////
template <typename _CharT>
inline bool is_quotation_mark (_CharT ch, parse_policy_set const & parse_policy)
{
    return (ch == '"'
            || (parse_policy.test(allow_single_quote_mark) && ch == '\''));
}

////////////////////////////////////////////////////////////////////////////////
// advance_sequence
// Based on pfs/algo/advance.hpp:advance_sequence
////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Advance by sequence of charcters.
 * @param pos On input - first position, on output - last good position.
 * @param last End of sequence position.
 * @return @c true if advanced by all character sequence [first2, last2),
 *      otherwise returns @c false.
 */
template <typename _ForwardIterator1, typename _ForwardIterator2>
inline bool advance_sequence (_ForwardIterator1 & pos, _ForwardIterator1 last
        , _ForwardIterator2 first2, _ForwardIterator2 last2)
{
    auto p = pos;

    while (p != last && first2 != last2 && *p++ == *first2++)
        ;

    if (first2 == last2) {
        pos = p;
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
// advance_null
////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Advance by 'null' string.
 * @param pos On input - first position, on output - last good position.
 * @param last End of sequence position.
 * @return @c true if advanced by at least one position, otherwise @c false.
 *
 * @note Grammar
 * null  = %x6e.75.6c.6c      ; null
 */
template <typename _ForwardIterator>
inline bool advance_null (_ForwardIterator & pos, _ForwardIterator last)
{
    std::string s{"null"};
    return advance_sequence(pos, last, s.begin(), s.end());
}

////////////////////////////////////////////////////////////////////////////////
// advance_true
////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Advance by 'true' string.
 * @param pos On input - first position, on output - last good position.
 * @param last End of sequence position.
 * @return @c true if advanced by at least one position, otherwise @c false.
 *
 * @note Grammar
 * true  = %x74.72.75.65      ; true
 */
template <typename _ForwardIterator>
inline bool advance_true (_ForwardIterator & pos, _ForwardIterator last)
{
    std::string s{"true"};
    return advance_sequence(pos, last, s.begin(), s.end());
}

////////////////////////////////////////////////////////////////////////////////
// advance_false
////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Advance by 'false' string.
 * @param pos On input - first position, on output - last good position.
 * @param last End of sequence position.
 * @return @c true if advanced by at least one position, otherwise @c false.
 *
 * @note Grammar
 * true  = %x74.72.75.65      ; true
 */
template <typename _ForwardIterator>
inline bool advance_false (_ForwardIterator & pos, _ForwardIterator last)
{
    std::string s{"false"};
    return advance_sequence(pos, last, s.begin(), s.end());
}

////////////////////////////////////////////////////////////////////////////////
// advance_encoded_char
////////////////////////////////////////////////////////////////////////////////
/**
 * @note Grammar:
 * encoded_char = 4HEXDIG
 */
template <typename _ForwardIterator>
bool advance_encoded_char (_ForwardIterator & pos, _ForwardIterator last
        , int32_t & result)
{
    static constexpr int32_t multipliers[] = { 16 * 16 * 16, 16 * 16, 16, 1 };
    static constexpr int count = sizeof(multipliers) / sizeof(multipliers[0]);
    auto p = pos;
    int index = 0;

    result = 0;

    for (p = pos; p != last && is_hexdigit(*p) && index < count; ++p, ++index) {
        int32_t n = to_digit(*p, 16);
        result += n * multipliers[index];
    }

    if (index != count)
        return false;

    return compare_and_assign(pos, p);
}


////////////////////////////////////////////////////////////////////////////////
// advance_string
////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Advance by string.
 * @param pos On input - first position, on output - last good position.
 * @param last End of sequence position.
 * @return @c true if advanced by at least one position, otherwise @c false.
 *
 * @note Grammar
 *  string = quotation-mark *char quotation-mark
 *
 *  char = unescaped /
 *      escape (
 *          %x22 /          ; "    quotation mark  U+0022
 *          %x5C /          ; \    reverse solidus U+005C
 *          %x2F /          ; /    solidus         U+002F
 *          %x62 /          ; b    backspace       U+0008
 *          %x66 /          ; f    form feed       U+000C
 *          %x6E /          ; n    line feed       U+000A
 *          %x72 /          ; r    carriage return U+000D
 *          %x74 /          ; t    tab             U+0009
 *          %x75 4HEXDIG )  ; uXXXX                U+XXXX
 *
 *  escape = %x5C              ; \
 *  quotation-mark = %x22      ; "
 *      / %x27                 ; '  JSON5 specific
 *  unescaped = %x20-21 / %x23-5B / %x5D-10FFFF
 */
template <typename _ForwardIterator, typename _OutputIterator>
inline bool advance_string (_ForwardIterator & pos, _ForwardIterator last
        , parse_policy_set const & parse_policy
        , _OutputIterator output
        , error_code & ec)
{
    using char_type = typename std::remove_reference<decltype(*pos)>::type;

    auto p = pos;

    if (p == last)
        return false;

    if (!is_quotation_mark(*p, parse_policy))
        return false;

    auto quotation_mark = *p;

    ++p;

    if (p == last) {
        ec = make_error_code(errc::unbalanced_quote);
        return false;
    }

    // Check empty string
    if (*p == quotation_mark) {
        ++p;
        return compare_and_assign(pos, p);
    }

    bool escaped = false;
    bool encoded = false;

//     auto output = std::back_inserter(result);

    while (p != last && *p != quotation_mark) {
        if (encoded) {
            int32_t encoded_char = 0;

            if (!advance_encoded_char(p, last, encoded_char)) {
                ec = make_error_code(errc::bad_encoded_char);
                return false;
            }

            *output++ = char_type(encoded_char);

            encoded = false;
            continue;
        }

        if (!escaped) {
            if (*p == '\\') { // escape character
                escaped = true;
            } else {
                *output++ = *p;
            }
        } else {
            auto escaped_char = *p;

            switch (escaped_char) {
                case '"':
                case '\\':
                case '/': break;

                case '\'':
                    if (quotation_mark != '\'') {
                        ec = make_error_code(errc::bad_escaped_char);
                        return false;
                    }
                    break;

                case 'b': escaped_char = '\b'; break;
                case 'f': escaped_char = '\f'; break;
                case 'n': escaped_char = '\n'; break;
                case 'r': escaped_char = '\r'; break;
                case 't': escaped_char = '\t'; break;

                case 'u': encoded = true; break;

                default:
                    if (!parse_policy.test(allow_any_char_escaped)) {
                        ec = make_error_code(errc::bad_escaped_char);
                        return false;
                    }
            }

            if (!encoded) {
                *output++ = escaped_char;
            }

            // Finished process escaped sequence
            escaped = false;
        }

        ++p;
    }

    // ERROR: unquoted string
    if (p == last || *p != quotation_mark) {
        ec = make_error_code(errc::unbalanced_quote);
        return false;
    }

    // Skip quotation mark
    ++p;

    return compare_and_assign(pos, p);
}


////////////////////////////////////////////////////////////////////////////////
// advance_number
////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Advance by number.
 * @param pos On input - first position, on output - last good position.
 * @param last End of sequence position.
 * @return @c true if advanced by at least one position, otherwise @c false.
 *
 * @note Grammar
 * number = [ minus ] int [ frac ] [ exp ]
 * decimal-point = %x2E       ; .
 * digit1-9 = %x31-39         ; 1-9
 * e = %x65 / %x45            ; e E
 * exp = e [ minus / plus ] 1*DIGIT
 * frac = decimal-point 1*DIGIT
 * int = zero / ( digit1-9 *DIGIT )
 * minus = %x2D               ; -
 * plus = %x2B                ; +
 * zero = %x30                ; 0
 *
 * @note @a _NumberType traits:
 *      _NumberType & operator = (intmax_t)
 *      _NumberType & operator = (uintmax_t)
 *      _NumberType & operator = (double)
 */
template <typename _ForwardIterator, typename NumberPtr>
bool advance_number (_ForwardIterator & pos, _ForwardIterator last
        , parse_policy_set const & parse_policy
        , NumberPtr pnum
        , error_code & /*ec*/)
{
    auto p = pos;
    std::string numstr;
    int sign = 1;
    bool is_integer = true;

    ////////////////////////////////////////////////////////////////////////////
    // Advance sign
    ////////////////////////////////////////////////////////////////////////////
    if (p != last) {
        if (*p == '-') {
            sign = -1;
            numstr.push_back('-');
            ++p;
        } else if (*p == '+') {
            if (parse_policy.test(allow_positive_signed_number)) {
                ++p;
            } else {
                return false;
            }
        }
    }

    auto last_pos = p;

    ////////////////////////////////////////////////////////////////////////////
    // Advance integral part
    //
    // Mandatory
    // int = zero / ( digit1-9 *DIGIT )
    ////////////////////////////////////////////////////////////////////////////
    if (p != last) {
        if (*p == '0') {
            numstr.push_back('0');
            ++p;
        } else {
            while (p != last && is_digit(*p)) {
                numstr.push_back(*p);
                ++p;
            }
        }
    }

    // No digit found
    if (p == last_pos)
        return false;

    last_pos = p;

    ////////////////////////////////////////////////////////////////////////////
    // Fractional part
    //
    // Optional
    // frac = decimal-point 1*DIGIT
    ////////////////////////////////////////////////////////////////////////////
    if (p != last) {
        if (*p == '.') {
            is_integer = false;
            numstr.push_back(locale_decimal_point());

            ++p;

            if (p == last)
                return false;

            if (!is_digit(*p))
                return false;

            numstr.push_back(*p);
            ++p;

            while (p != last && is_digit(*p)) {
                numstr.push_back(*p);
                ++p;
            }
        }
    }

    last_pos = p;

    ////////////////////////////////////////////////////////////////////////////
    // Exponential part
    //
    // Optional
    // exp = e [ minus / plus ] 1*DIGIT
    ////////////////////////////////////////////////////////////////////////////
    if (p != last) {
        if (*p == 'e' || *p == 'E') {
            is_integer = false;
            numstr.push_back('e');
            ++p;

            if (p != last) {
                if (*p == '-') {
                    numstr.push_back('-');
                    ++p;
                } else if (*p == '+') {
                    ++p;
                }
            }

            if (p == last)
                return false;

            if (!is_digit(*p))
                return false;

            numstr.push_back(*p);
            ++p;

            while (p != last && is_digit(*p)) {
                numstr.push_back(*p);
                ++p;
            }
        }
    }

    bool integer_accepted = false;

    if (is_integer) {
        if (sign > 0) {
            uintmax_t n = 0;

            if (strtouinteger(n, numstr)) {
                *pnum = n;
                integer_accepted = true;
            }
        } else {
            intmax_t n = 0;

            if (strtointeger(n, numstr)) {
                *pnum = n;
                integer_accepted = true;
            }
        }
    }

    if (!integer_accepted) {
        double n = 0;
        if (!strtoreal(n, numstr)) {
            return false;
        }

        *pnum = n;
    }

    return compare_and_assign(pos, p);
}

////////////////////////////////////////////////////////////////////////////////
// Forward declaration for advance_value
////////////////////////////////////////////////////////////////////////////////
template <typename _ForwardIterator, typename CallbacksType>
bool advance_value (_ForwardIterator & pos, _ForwardIterator last
        , parse_policy_set const & parse_policy
        , CallbacksType callbacks);

////////////////////////////////////////////////////////////////////////////////
// advance_value_separator
////////////////////////////////////////////////////////////////////////////////
/**
 *  begin-array     = ws %x5B ws  ; [ left square bracket
 *  begin-object    = ws %x7B ws  ; { left curly bracket
 *  end-array       = ws %x5D ws  ; ] right square bracket
 *  end-object      = ws %x7D ws  ; } right curly bracket
 *  name-separator  = ws %x3A ws  ; : colon
 *  value-separator = ws %x2C ws  ; , comma
 */
template <typename _ForwardIterator>
bool advance_delimiter_char (_ForwardIterator & pos, _ForwardIterator last
        , typename std::remove_reference<decltype(*pos)>::type delim)
{
    auto p = pos;

    advance_whitespaces(p, last);

    if (p == last)
        return false;

    if (*p != delim)
        return false;

    ++p;
    advance_whitespaces(p, last);

    return compare_and_assign(pos, p);
}

template <typename _ForwardIterator>
inline bool advance_begin_array (_ForwardIterator & pos, _ForwardIterator last)
{
    using char_type = typename std::remove_reference<decltype(*pos)>::type;
    return advance_delimiter_char(pos, last, char_type{'['});
}

template <typename _ForwardIterator>
inline bool advance_begin_object (_ForwardIterator & pos, _ForwardIterator last)
{
    using char_type = typename std::remove_reference<decltype(*pos)>::type;
    return advance_delimiter_char(pos, last, char_type{'{'});
}

template <typename _ForwardIterator>
inline bool advance_end_array (_ForwardIterator & pos, _ForwardIterator last)
{
    using char_type = typename std::remove_reference<decltype(*pos)>::type;
    return advance_delimiter_char(pos, last, char_type{']'});
}

template <typename _ForwardIterator>
inline bool advance_end_object (_ForwardIterator & pos, _ForwardIterator last)
{
    using char_type = typename std::remove_reference<decltype(*pos)>::type;
    return advance_delimiter_char(pos, last, char_type{'}'});
}

template <typename _ForwardIterator>
inline bool advance_name_separator (_ForwardIterator & pos, _ForwardIterator last)
{
    using char_type = typename std::remove_reference<decltype(*pos)>::type;
    return advance_delimiter_char(pos, last, char_type{':'});
}

template <typename _ForwardIterator>
inline bool advance_value_separator (_ForwardIterator & pos, _ForwardIterator last)
{
    using char_type = typename std::remove_reference<decltype(*pos)>::type;
    return advance_delimiter_char(pos, last, char_type{','});
}

////////////////////////////////////////////////////////////////////////////////
// advance_array
////////////////////////////////////////////////////////////////////////////////
/**
 *
 * @note Grammar:
 * array = begin-array [ value *( value-separator value ) ] end-array
 * begin-array     = ws %x5B ws  ; [ left square bracket
 * end-array       = ws %x5D ws  ; ] right square bracket
 * value-separator = ws %x2C ws  ; , comma
 * value = false / null / true / object / array / number / string
 */
template <typename _ForwardIterator, typename CallbacksType>
bool advance_array (_ForwardIterator & pos, _ForwardIterator last
        , parse_policy_set const & parse_policy
        , CallbacksType callbacks)
{
    auto p = pos;

    if (!advance_begin_array(p, last))
        return false;

    callbacks.on_begin_array();

    // Check empty array
    if (advance_end_array(p, last)) {
        callbacks.on_end_array();
    } else {
        do {
            if (!advance_value(p, last, parse_policy, callbacks))
                return false;
        } while(advance_value_separator(p, last));

        if (!advance_end_array(p, last)) {
            callbacks.on_error(make_error_code(errc::unbalanced_array_bracket));
            return false;
        }

        callbacks.on_end_array();
    }

    return compare_and_assign(pos, p);
}

////////////////////////////////////////////////////////////////////////////////
// advance_member
////////////////////////////////////////////////////////////////////////////////
// Note: std::size() available since C++17
template <typename C>
inline auto size (C const & c) -> decltype(c.size())
{
    return c.size();
}

/**
 * @note Grammar:
 * member = string name-separator value
 * name-separator  = ws %x3A ws  ; : colon
 * value = false / null / true / object / array / number / string
 */
template <typename _ForwardIterator, typename CallbacksType>
bool advance_member (_ForwardIterator & pos, _ForwardIterator last
        , parse_policy_set const & parse_policy
        , CallbacksType callbacks)
{
    auto p = pos;

    typename CallbacksType::string_type name;
    error_code ec;

    if (!advance_string(p, last, parse_policy, std::back_inserter(name), ec)) {
        // Error while parsing value
        if (ec) {
            callbacks.on_error(ec);
            return false;
        } else {
            // is not a string
            callbacks.on_error(make_error_code(errc::bad_member_name));
        }
    }

    // Member name must be non-empty
    if (size(name) == 0) {
        callbacks.on_error(make_error_code(errc::bad_member_name));
        return false;
    }

    if (!advance_name_separator(p, last))
        return false;

    callbacks.on_member_name(std::move(name));

    if (!advance_value(p, last, parse_policy, callbacks))
        return false;

    return compare_and_assign(pos, p);
}

////////////////////////////////////////////////////////////////////////////////
// advance_object
////////////////////////////////////////////////////////////////////////////////
/**
 * @details _ObjectType traits:
 *  - provides @code key_type @endcode typename
 *  - provides @code mapped_type @endcode typename
 *  - provides @code emplace(std::pair<key_type, mapped_type> &&) @endcode method
 *
 * @note Grammar:
 * object = begin-object [ member *( value-separator member ) ] end-object
 * member = string name-separator value
 * begin-object    = ws %x7B ws  ; { left curly bracket
 * end-object      = ws %x7D ws  ; } right curly bracket
 * name-separator  = ws %x3A ws  ; : colon
 * value-separator = ws %x2C ws  ; , comma
 * value = false / null / true / object / array / number / string
 */
template <typename _ForwardIterator, typename CallbacksType>
bool advance_object (_ForwardIterator & pos, _ForwardIterator last
        , parse_policy_set const & parse_policy
        , CallbacksType callbacks)
{
    auto p = pos;

    if (!advance_begin_object(p, last))
        return false;

    callbacks.on_begin_object();

    // Check empty object
    if (advance_end_object(p, last)) {
        callbacks.on_end_object();
    } else {
        do {
            if (!advance_member(p, last, parse_policy, callbacks))
                return false;
        } while(advance_value_separator(p, last));

        if (!advance_end_object(p, last)) {
            callbacks.on_error(make_error_code(errc::unbalanced_object_bracket));
            return false;
        }

        callbacks.on_end_object();
    }

    return compare_and_assign(pos, p);
}

////////////////////////////////////////////////////////////////////////////////
// advance_value
//
// Specialization: no, for values of any type
////////////////////////////////////////////////////////////////////////////////
template <typename _ForwardIterator, typename CallbacksType>
bool advance_value (_ForwardIterator & pos, _ForwardIterator last
        , parse_policy_set const & parse_policy
        , CallbacksType callbacks)
{
    auto p = pos;

    // Skip head witespaces
    advance_whitespaces(p, last);

    do {
        // Array marker
        if (*p == '[') {
            if (advance_array(p, last, parse_policy, callbacks)) {
                break;
            }
        }

        // Object marker
        if (*p == '{') {
            if (advance_object(p, last, parse_policy, callbacks)) {
                break;
            }
        }

        if (advance_null(p, last)) {
            callbacks.on_null();
            break;
        }

        if (advance_true(p, last)) {
            callbacks.on_true();
            break;
        }

        if (advance_false(p, last)) {
            callbacks.on_false();
            break;
        }

        typename CallbacksType::number_type num;
        error_code ec;

        if (advance_number(p, last, parse_policy, & num, ec)) {
            callbacks.on_number(std::move(num));
            break;
        } else if (ec) {
            callbacks.on_error(ec);
            return false;
        }

        typename CallbacksType::string_type str;

        if (advance_string(p, last, parse_policy, std::back_inserter(str), ec)) {
            callbacks.on_string(std::move(str));
            break;
        } else if (ec) {
            callbacks.on_error(ec);
            return false;
        }

        // Not JSON sequence
        callbacks.on_error(make_error_code(errc::bad_json_sequence));
        return false;
    } while (false);

    // Skip tail witespaces
    advance_whitespaces(p, last);

    return compare_and_assign(pos, p);
}

////////////////////////////////////////////////////////////////////////////////
// advance_json
////////////////////////////////////////////////////////////////////////////////
template <typename _ForwardIterator, typename CallbacksType>
bool advance_json (_ForwardIterator & pos, _ForwardIterator last
        , parse_policy_set const & parse_policy
        , CallbacksType callbacks)
{
    // Doublicated advance_value

    auto p = pos;

    // Skip head witespaces
    advance_whitespaces(p, last);

    do {
        // Array marker
        if (*p == '[') {
            if (!parse_policy.test(allow_array_root_element)) {
                callbacks.on_error(make_error_code(errc::forbidden_root_element));
                return false;
            }

            if (advance_array(p, last, parse_policy, callbacks))
                break;
        } else if (*p == '{') {  // Object marker
            if (!parse_policy.test(allow_object_root_element)) {
                callbacks.on_error(make_error_code(errc::forbidden_root_element));
                return false;
            }

            if (advance_object(p, last, parse_policy, callbacks))
                break;
        } else if (advance_null(p, last)) {
            if (!parse_policy.test(allow_null_root_element)) {
                callbacks.on_error(make_error_code(errc::forbidden_root_element));
                return false;
            }

            callbacks.on_null();
            break;
        } else if (advance_true(p, last)) {
            if (!parse_policy.test(allow_boolean_root_element)) {
                callbacks.on_error(make_error_code(errc::forbidden_root_element));
                return false;
            }

            callbacks.on_true();
            break;
        } else if (advance_false(p, last)) {
            if (!parse_policy.test(allow_boolean_root_element)) {
                callbacks.on_error(make_error_code(errc::forbidden_root_element));
                return false;
            }

            callbacks.on_false();
            break;
        } else {
            typename CallbacksType::number_type num;
            typename CallbacksType::string_type str;

            error_code ec;

            if (advance_number(p, last, parse_policy, & num, ec)) {
                if (!parse_policy.test(allow_number_root_element)) {
                    callbacks.on_error(make_error_code(errc::forbidden_root_element));
                    return false;
                }

                callbacks.on_number(std::move(num));
                break;
            } else if (ec) {
                callbacks.on_error(ec);
                return false;
            }

            if (advance_string(p, last, parse_policy, std::back_inserter(str), ec)) {
                if (!parse_policy.test(allow_string_root_element)) {
                    callbacks.on_error(make_error_code(errc::forbidden_root_element));
                    return false;
                }

                callbacks.on_string(std::move(str));
                break;
            } else if (ec) {
                callbacks.on_error(ec);
                return false;
            }
        }

        // Not JSON sequence
        callbacks.on_error(make_error_code(errc::bad_json_sequence));
        return false;
    } while (false);

    // Skip tail witespaces
    advance_whitespaces(p, last);

    return compare_and_assign(pos, p);
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
template <typename _ForwardIterator, typename CallbacksType>
inline _ForwardIterator parse (_ForwardIterator first
        , _ForwardIterator last
        , parse_policy_set const & parse_policy
        , CallbacksType callbacks)
{
    auto pos = first;

    if (advance_json(pos, last, parse_policy, callbacks))
        return pos;

    return first;
}

/**
 *
 */
template <typename _ForwardIterator, typename CallbacksType>
inline _ForwardIterator parse (_ForwardIterator first
        , _ForwardIterator last
        , CallbacksType callbacks)
{
    return parse(first, last, default_policy(), callbacks);
}

////////////////////////////////////////////////////////////////////////////////
// parse_array
////////////////////////////////////////////////////////////////////////////////
template <typename _ForwardIterator, typename ArrayType>
typename std::enable_if<std::is_arithmetic<typename ArrayType::value_type>::value, _ForwardIterator>::type
parse_array (_ForwardIterator first
        , _ForwardIterator last
        , parse_policy_set const & parse_policy
        , ArrayType & arr
        , error_code & ec)
{
    using value_type = typename ArrayType::value_type;
    //                   v----------No matter the string type here
    basic_callbacks<std::string, value_type> callbacks;
    callbacks.on_error  = [& ec] (error_code const & e) { ec = e; };
    callbacks.on_true   = [& arr] { arr.emplace_back(static_cast<value_type>(true)); };
    callbacks.on_false  = [& arr] { arr.emplace_back(static_cast<value_type>(false)); };
    callbacks.on_number = [& arr] (value_type && n) { arr.emplace_back(std::forward<value_type>(n)); };
    return parse(first, last, parse_policy, callbacks);
}

template <typename _StringType>
struct is_string;

template <>
struct is_string<std::string> : std::integral_constant<bool, true> {};

template <typename _ForwardIterator, typename ArrayType>
typename std::enable_if<is_string<typename ArrayType::value_type>::value, _ForwardIterator>::type
parse_array (_ForwardIterator first
        , _ForwardIterator last
        , parse_policy_set const & parse_policy
        , ArrayType & arr
        , error_code & ec)
{
    using string_type = typename ArrayType::value_type;
    //                            v------------ No matter the number type here
    basic_callbacks<string_type, int> callbacks;
    callbacks.on_error  = [& ec] (error_code const & e) { ec = e; };
    callbacks.on_string = [& arr] (string_type && s) {
        arr.emplace_back(std::forward<string_type>(s));
    };
    return parse(first, last, parse_policy, callbacks);
}

template <typename _ForwardIterator, typename ArrayType>
inline _ForwardIterator parse_array (_ForwardIterator first
        , _ForwardIterator last
        , ArrayType & arr
        , error_code & ec)
{
    return parse_array(first, last, default_policy(), arr, ec);
}

template <typename _ForwardIterator, typename ArrayType>
inline _ForwardIterator parse_array (_ForwardIterator first
        , _ForwardIterator last
        , ArrayType & arr)
{
    error_code ec;
    auto pos = parse_array(first, last, arr, ec);
    if (ec)
        throw std::system_error(ec);
    return pos;
}

////////////////////////////////////////////////////////////////////////////////
// parse_object
////////////////////////////////////////////////////////////////////////////////
template <typename _ForwardIterator, typename _ObjectType>
typename std::enable_if<is_string<typename _ObjectType::key_type>::value
        && std::is_arithmetic<typename _ObjectType::mapped_type>::value, _ForwardIterator>::type
parse_object (_ForwardIterator first
        , _ForwardIterator last
        , parse_policy_set const & parse_policy
        , _ObjectType & obj
        , error_code & ec)
{
    using value_type = typename _ObjectType::mapped_type;
    using string_type = typename _ObjectType::key_type;

    basic_callbacks<string_type, value_type> callbacks;
    string_type member_name;

    callbacks.on_error  = [& ec] (error_code const & e) { ec = e; };
    callbacks.on_member_name  = [& member_name] (string_type && name) { member_name = std::move(name); };
    callbacks.on_true   = [& obj, & member_name] { obj[member_name] = true; };
    callbacks.on_false  = [& obj, & member_name] { obj[member_name] = false; };
    callbacks.on_number = [& obj, & member_name] (value_type && n) { obj[member_name] = std::forward<value_type>(n); };
    return parse(first, last, parse_policy, callbacks);
}

template <typename _ForwardIterator, typename _ObjectType>
typename std::enable_if<is_string<typename _ObjectType::key_type>::value
        && is_string<typename _ObjectType::mapped_type>::value, _ForwardIterator>::type
parse_object (_ForwardIterator first
        , _ForwardIterator last
        , parse_policy_set const & parse_policy
        , _ObjectType & obj
        , error_code & ec)
{
    using value_type = typename _ObjectType::mapped_type;
    using string_type = typename _ObjectType::key_type;

    basic_callbacks<string_type, value_type> callbacks;
    string_type member_name;

    callbacks.on_error  = [& ec] (error_code const & e) { ec = e; };
    callbacks.on_member_name  = [& member_name] (string_type && name) { member_name = std::move(name); };
    callbacks.on_string = [& obj, & member_name] (string_type && s) {
            obj[member_name] = std::forward<string_type>(s);
    };
    return parse(first, last, parse_policy, callbacks);
}

template <typename _ForwardIterator, typename _ObjectType>
inline _ForwardIterator parse_object (_ForwardIterator first
        , _ForwardIterator last
        , _ObjectType & obj
        , error_code & ec)
{
    return parse_object(first, last, default_policy(), obj, ec);
}

template <typename _ForwardIterator, typename _ObjectType>
inline _ForwardIterator parse_object (_ForwardIterator first
        , _ForwardIterator last
        , _ObjectType & obj)
{
    error_code ec;
    auto pos = parse_object(first, last, obj, ec);
    if (ec)
        throw std::system_error(ec);
    return pos;
}

#endif // __COMMENT__

}}} // // namespace pfs::parser::abnf
