////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [pfs-parser](https://github.com/semenovf/pfs-parser) library.
//
// Changelog:
//      2021.02.25 Initial version
////////////////////////////////////////////////////////////////////////////////
#include "rfc3986.hpp"
#include "read_file.hpp"
#include <iostream>
#include <string>

static std::string grammar_file {"./uri-rfc3986.grammar"};

int main ()
{
    using pfs::parser::abnf::advance_rulelist;
    using grammar::rfc3986::forward_iterator;
    using grammar::rfc3986::context;

    auto source = utils::read_file(grammar_file);

    if (source.empty()) {
        std::cerr << "ERROR: reading file failure or it is empty" << std::endl;
        return false;
    }

    context ctx;
    auto first = forward_iterator {source.begin()};
    auto last  = forward_iterator {source.end()};
    auto result = advance_rulelist(first, last, ctx);

    if (!result || first != last) {
        std::cerr << "ERROR: RFC-3986 parsing failed at " << first.lineno()
            << " line: "
            << ctx.lastError()
            << std::endl;
        return EXIT_FAILURE;
    } else {
        std::cout << "RFC-3986 parsing successed" << std::endl;
    }

//         CHECK_MESSAGE(result, item.filename);
//         CHECK((first == last));
//         CHECK(ctx.rulenames == item.rulenames);
    return 0;
}
