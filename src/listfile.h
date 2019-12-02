/*
 * listfile.h
 *
 *  Created on: 24/04/2013
 *      Author: nicholas
 */

#ifndef LISTFILE_H_
#define LISTFILE_H_
#include <string>
#include <iostream>
#include <vector>

namespace cmake
{

struct comment_t
{
    std::vector<std::string> cmt;
};

comment_t comment(const std::string& text)
{
    comment_t cmt;

    std::istringstream s(text);
    std::string line;
    while (std::getline(s, line))
        cmt.cmt.push_back(line);

    return cmt;
}

std::ostream& operator<<(std::ostream& os, const comment_t& cmt)
{
    for(auto& l : cmt.cmt)
        os << '#' << l << '\n';
    return os;
}

struct variable_t
{
    std::string var;
};

struct argument_t
{
    bool quoted;
    std::string arg;
};

template <typename... Args>
struct command_t
{
    std::string name;
    std::tuple<Args...> args;
};

template <typename... Args>
std::ostream& operator<<(std::ostream& os, const command_t<Args...>& cmd)
{
    os << cmd.name << " (";
    // TODO write args
    os << ")\n";
    return os;
}

template <typename... Args>
command_t<Args...> command(const std::string& name, Args... args)
{
    return {name, args...};
}

}


#endif /* LISTFILE_H_ */
