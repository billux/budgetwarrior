//=======================================================================
// Copyright (c) 2013-2014 Baptiste Wicht.
// Distributed under the terms of the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

#include <cstdio>
#include <fstream>

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include "cpp_utils/assert.hpp"

#include "utils.hpp"
#include "budget_exception.hpp"
#include "config.hpp"
#include "expenses.hpp"
#include "earnings.hpp"

unsigned short budget::terminal_width(){
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_col;
}

unsigned short budget::terminal_height(){
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_row;
}

bool budget::file_exists(const std::string& name){
    std::ifstream f(name.c_str());
    if (f.good()) {
        f.close();
        return true;
    } else {
        f.close();
        return false;
    }
}

bool budget::folder_exists(const std::string& name){
    struct stat sb;
    return stat(name.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode);
}

std::vector<std::string>& budget::split(const std::string& s, char delim, std::vector<std::string>& elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> budget::split(const std::string& s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}
