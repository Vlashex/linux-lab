#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

// utils.h
namespace Utils
{
    std::vector<std::string> split(const std::string &line);
    std::string expandTilde(const std::string &path);
}

#endif // UTILS_H
