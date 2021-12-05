////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [pfs-parser](https://github.com/semenovf/pfs-parser) library.
//
// Changelog:
//      2021.02.25 Initial version.
//      2021.04.14 Apply syntax tree.
////////////////////////////////////////////////////////////////////////////////
#include "rfc3986.hpp"
#include "read_file.hpp"
#include "pfs/parser/abnf/syntax_tree.hpp"
#include <iostream>
#include <string>

static std::string grammar_file {"./uri-rfc3986.grammar"};

int main ()
{
    auto source = utils::read_file(grammar_file);

    if (source.empty()) {
        // LCOV_EXCL_START
        std::cerr << "ERROR: " << grammar_file << ": reading file failure or it is empty" << std::endl;
        return EXIT_FAILURE;
        // LCOV_EXCL_STOP
    }

    auto first = source.begin();
    auto last  = source.end();
    auto syntax_tree = pfs::parser::abnf::parse<std::string>(first, last);

    if (syntax_tree.error_code()) {
        std::cerr << "ERROR: parse failed at line "
            << syntax_tree.error_line()
            << ": " << syntax_tree.error_code();

        if (!syntax_tree.error_text().empty())
            std::cerr << ": " << syntax_tree.error_text();

        std::cerr << std::endl;
    } else {
        grammar::rfc3986::visitor v;
        syntax_tree.traverse(std::move(v));
    }

    if (first != last)
        std::cerr << "ERROR: parse is incomplete" << std::endl;


//     using pfs::parser::abnf::advance_rulelist;
//     using grammar::rfc3986::forward_iterator;
//     using grammar::rfc3986::context;
//
//     auto source = utils::read_file(grammar_file);
//
//     if (source.empty()) {
//         std::cerr << "ERROR: reading file failure or it is empty" << std::endl;
//         return false;
//     }
//
//     context ctx;
//     auto first = forward_iterator {source.begin()};
//     auto last  = forward_iterator {source.end()};
//     auto result = advance_rulelist(first, last, ctx);
//
//     if (!result || first != last) {
//         std::cerr << "ERROR: RFC-3986 parsing failed at " << first.lineno()
//             << " line: "
//             << ctx.lastError()
//             << std::endl;
//         return EXIT_FAILURE;
//     } else {
//         std::cout << "RFC-3986 parsing successed" << std::endl;
//     }
//
// //         CHECK_MESSAGE(result, item.filename);
// //         CHECK((first == last));
// //         CHECK(ctx.rulenames == item.rulenames);
    return EXIT_SUCCESS;
}
