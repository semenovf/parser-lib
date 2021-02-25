////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [pfs-parser](https://github.com/semenovf/pfs-parser) library.
//
// Changelog:
//      2021.02.14 Initial version
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <fstream>
#include <iostream>
#include <string>

namespace utils {

inline std::string read_file (std::string const & filename)
{
    // LCOV_EXCL_START
    std::ifstream ifs(filename.c_str(), std::ios::binary | std::ios::ate);

    if (!ifs.is_open()) {
        std::cerr << "Open file " << filename << " failure\n";
        return std::string{};
    }

    std::noskipws(ifs);

    // See examples at https://en.cppreference.com/w/cpp/io/basic_istream/read
    // and http://www.cplusplus.com/reference/istream/istream/read/
    ifs.seekg(0, ifs.end);
    auto size = ifs.tellg();
    std::string str(size, '\0'); // construct string to stream size
    ifs.seekg(0);

    ifs.read(& str[0], size);

    if (ifs) {
        ; // success
    } else {
        str.clear(); // error
    }

    ifs.close();
    return str;
    // LCOV_EXCL_STOP
}

} // namespace utils
