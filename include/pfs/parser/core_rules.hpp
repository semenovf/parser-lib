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

/**
 * Helper function assigns @a b to @a a if @a a != @a b.
 */
template <typename ForwardIterator>
inline bool compare_and_assign (ForwardIterator & a, ForwardIterator b)
{
    if (a != b) {
        a = b;
        return true;
    }

    return false;
}

/**
 * @return @c true if @a ch is alpha (A-Z / a-z), otherwise @c false.
 *
 * @note Grammar
 * ALPHA = %x41-5A / %x61-7A ; A-Z / a-z
 */
template <typename CharType>
inline bool is_alpha (CharType ch)
{
    auto a = uint32_t(ch) >= uint32_t(CharType('\x41'))
        &&   uint32_t(ch) <= uint32_t(CharType('\x5A'));

    return a || (uint32_t(ch) >= uint32_t(CharType('\x61')) && uint32_t(ch) <= uint32_t(CharType('\x7A')));
}

/**
 * @return @c true if @a ch is bit ("0" / "1"), otherwise @c false.
 *
 * @note Grammar
 * BIT = "0" / "1"
 */
template <typename CharType>
inline bool is_bit (CharType ch)
{
    return uint32_t(ch) == uint32_t(CharType('0')) || uint32_t(ch) == uint32_t(CharType('1'));
}

/**
 * @return @c true if @a ch is any 7-bit US-ASCII character, excluding NUL,
 *         otherwise @c false.
 *
 * @note Grammar
 * CHAR = %x01-7F ; any 7-bit US-ASCII character, excluding NUL
 */
template <typename CharType>
inline bool is_ascii_char (CharType ch)
{
    return uint32_t(ch) >= uint32_t(CharType('\x01'))
        && uint32_t(ch) <= uint32_t(CharType('\x7F'));
}

/**
 * @return @c true if @a ch is carriage return character (%x0D), otherwise @c false.
 *
 * @note Grammar
 * CR = %x0D ; carriage return
 */
template <typename CharType>
inline bool is_cr_char (CharType ch)
{
    return ch == CharType('\x0D');
}

/**
 * @return @c true if @a ch is linedeed character (%x0A), otherwise @c false.
 *
 * @note Grammar
 * LF = %x0A ; linefeed
 */
template <typename CharType>
inline bool is_lf_char (CharType ch)
{
    return ch == CharType('\x0A');
}

/**
 * @brief Advance by Internet standard newline.
 *
 * @param pos On input - first position, on output - last good position.
 * @param last End of sequence position.
 * @return @c true if advanced by newline sequence, otherwise @c false.
 *
 * @note Grammar
 * CRLF = CR LF ; Internet standard newline
 */
template <typename ForwardIterator>
bool advance_internet_newline (ForwardIterator & pos, ForwardIterator last)
{
    using char_type = typename std::remove_reference<decltype(*pos)>::type;

    auto p = pos;

    if (p == last)
        return false;

    if (!is_cr_char(*p))
        return false;

    ++p;

    if (!is_lf_char(*p))
        return false;

    ++p;

    return compare_and_assign(pos, p);
}

/**
 * @brief Advance by newline.
 *
 * @param pos On input - first position, on output - last good position.
 * @param last End of sequence position.
 * @return @c true if advanced by newline sequence, otherwise @c false.
 *
 * @note Grammar
 * WIN_NL  = CR LF ; '\r\n'
 * UNIX_NL = LF    ; '\n'
 * MAC_NL  = CR    ; '\r'
 * NL      = WIN_NL / UNIX_NL / MAC_NL
 */
template <typename ForwardIterator>
bool advance_newline (ForwardIterator & pos, ForwardIterator last)
{
    using char_type = typename std::remove_reference<decltype(*pos)>::type;

    auto p = pos;

    if (p == last)
        return false;

    if (is_cr_char(*p)) {
        ++p;

        if (p != last && is_lf_char(*p))
            ++p;
    } else if (is_lf_char(*p)) {
        ++p;
    } else {
        return false;
    }

    return compare_and_assign(pos, p);
}

/**
 * @return @c true if @a ch is control char, otherwise @c false.
 *
 * @note Grammar
 * CTL = %x00-1F / %x7F ; controls
 */
template <typename CharType>
inline bool is_control_char (CharType ch)
{
    auto a = uint32_t(ch) >= uint32_t(CharType('\x00'))
        &&   uint32_t(ch) <= uint32_t(CharType('\x1F'));

    return a || (ch == CharType('\x7F'));
}

/**
 * @return @c true if @a ch is decimal digit char, otherwise @c false.
 *
 * @note Grammar
 * DIGIT = %x30-39 ; 0-9
 */
template <typename CharType>
inline bool is_digit_char (CharType ch)
{
    return uint32_t(ch) >= uint32_t(CharType('\x30'))
        && uint32_t(ch) <= uint32_t(CharType('\x39'));
}

/**
 * @return @c true if @a ch is hexadecimal digit char, otherwise @c false.
 *
 * @note Grammar
 * HEXDIG = DIGIT / "A" / "B" / "C" / "D" / "E" / "F"
 */
template <typename CharType>
inline bool is_hexdigit_char (CharType ch)
{
    return is_digit_char(ch)
        || (uint32_t(ch) >= uint32_t(CharType('A')) && uint32_t(ch) <= uint32_t(CharType('F')))
        || (uint32_t(ch) >= uint32_t(CharType('a')) && uint32_t(ch) <= uint32_t(CharType('f')));
}

/**
 * @return @c true if @a ch is double quote character (%x22), otherwise @c false.
 *
 * @note Grammar
 * DQUOTE = %x22 ; " (Double Quote)
 */
template <typename CharType>
inline bool is_dquote_char (CharType ch)
{
    return ch == CharType('\x22');
}

/**
 * @return @c true if @a ch is horizontal tab character (%x09), otherwise @c false.
 *
 * @note Grammar
 * HTAB = %x09 ; horizontal tab
 */
template <typename CharType>
inline bool is_htab_char (CharType ch)
{
    return ch == CharType('\x09');
}

/**
 * @return @c true if @a ch is 8 bits of data character, otherwise @c false.
 *
 * @note Grammar
 * OCTET = %x00-FF ; 8 bits of data
 */
template <typename CharType>
inline bool is_octet_char (CharType ch)
{
    return uint32_t(ch) >= uint32_t(CharType('\x00'))
        && uint32_t(ch) <= uint32_t(CharType('\xFF'));
}

/**
 * @return @c true if @a ch is space character (%x20), otherwise @c false.
 *
 * @note Grammar
 * SP =  %x20
 */
template <typename CharType>
inline bool is_space_char (CharType ch)
{
    return ch == CharType('\x20');
}

/**
 * @return @c true if @a ch is visible (printing) character (%x21-7E),
 *         otherwise @c false.
 *
 * @note Grammar
 * VCHAR = %x21-7E ; visible (printing) characters
 */
template <typename CharType>
inline bool is_visible_char (CharType ch)
{
    return uint32_t(ch) >= uint32_t(CharType('\x21'))
        && uint32_t(ch) <= uint32_t(CharType('\x7E'));
}

/**
 * @return @c true if @a ch is white space character (space or horizontal tab),
 *         otherwise @c false.
 *
 * @note Grammar
 * WSP = SP / HTAB ; white space
 */
template <typename CharType>
inline bool is_whitespace_char (CharType ch)
{
    return ch == CharType('\x20') || ch == CharType('\x09');
}

/**
 * @brief Advance by sequence of whitespace characters.
 *
 * @param pos On input - first position, on output - last good position.
 * @param last End of sequence position.
 * @return @c true if advanced by at least one position, otherwise @c false.
 *
 * @note Grammar
 * LWSP = *(WSP / CRLF WSP) ; Use of this linear-white-space rule permits lines containing only white
 *                          ; space that are no longer legal in mail headers and have caused
 *                          ; interoperability problems in other contexts.
 *                          ; Do not use when defining mail headers and use with caution in
 *                          ; other contexts.
 */
template <typename ForwardIterator>
bool advance_linear_whitespace (ForwardIterator & pos, ForwardIterator last)
{
    auto p = pos;

    while (p != last) {
        if (is_whitespace_char(*p))
            ++p;
        else if (advance_newline(p, last))
            ;
        else
            break;
    }

    return compare_and_assign(pos, p);
}

}} // namespace pfs::parser
