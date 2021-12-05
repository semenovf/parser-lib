////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [pfs-parser](https://github.com/semenovf/pfs-parser) library.
//
// Changelog:
//      2021.04.14 Initial version
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <chrono>
#include <ctime>
#include <locale>

// Return current localized date
std::string current_date ()
{
    try {
        auto now   = std::chrono::system_clock::now();
        auto ctime = std::chrono::system_clock::to_time_t(now);
        auto ltime = std::localtime(& ctime);

        std::locale loc;
        auto time_put = std::use_facet<std::time::put<char>>(loc);
    } catch (...) {
        ;
    }

    return std::string{};
}
