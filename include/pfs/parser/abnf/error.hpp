////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [pfs-parser](https://github.com/semenovf/pfs-parser) library.
//
// Changelog:
//      2021.01.16 Initial version
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <system_error>

namespace pfs {
namespace parser {

////////////////////////////////////////////////////////////////////////////////
// Error codes, category, exception class
////////////////////////////////////////////////////////////////////////////////
using error_code = std::error_code;

enum class errc
{
      success = 0

    // Parser errors
    , unbalanced_quote
    , bad_quoted_char
    , max_length_exceeded
    , bad_repeat_range

    // Syntax analyzer errors
    , rule_undefined
    , rulename_duplicated
};

class error_category : public std::error_category
{
public:
    virtual char const * name () const noexcept override
    {
        return "parser_category";
    }

    virtual std::string message (int ev) const override
    {
        switch (ev) {
            case static_cast<int>(errc::success):
                return std::string{"no error"};
            case static_cast<int>(errc::unbalanced_quote):
                return std::string{"unquoted string"};
            case static_cast<int>(errc::bad_quoted_char):
                return std::string{"bad quoted char"};
            case static_cast<int>(errc::max_length_exceeded):
                return std::string{"maximum string length exceeded"};
            case static_cast<int>(errc::bad_repeat_range):
                return std::string{"bad repeat range"};

            case static_cast<int>(errc::rule_undefined):
                return std::string{"rule undefined"};
            case static_cast<int>(errc::rulename_duplicated):
                return std::string{"rulename duplicated"};

            default: return std::string{"unknown parser error"};
        }
    }
};

inline std::error_category const & get_error_category ()
{
    static error_category instance;
    return instance;
}

inline std::error_code make_error_code (errc e)
{
    return std::error_code(static_cast<int>(e), get_error_category());
}

inline std::system_error make_exception (errc e)
{
    return std::system_error(make_error_code(e));
}

// inline std::system_error make_cast_exception (std::string const & from
//     , std::string const & to)
// {
//     return std::system_error(make_error_code(errc::type_error)
//             , std::string("can not cast from ") + from + " to " + to);
// }

}} // namespace pfs::parser

