#include "utils.h"

#include <string>
#include <vector>
#include <sstream>

namespace Utils
{

    std::vector<std::string> split(const std::string &line)
    {
        std::vector<std::string> tokens;
        std::string current;
        bool inQuotes = false;

        for (std::size_t i = 0; i < line.size(); ++i)
        {
            char c = line[i];

            if (c == '"')
            {
                inQuotes = !inQuotes;
                continue;
            }

            if (std::isspace(static_cast<unsigned char>(c)) && !inQuotes)
            {
                if (!current.empty())
                {
                    tokens.push_back(current);
                    current.clear();
                }
            }
            else
            {
                current.push_back(c);
            }
        }

        if (!current.empty())
        {
            tokens.push_back(current);
        }

        return tokens;
    }

    std::string expandTilde(const std::string &path)
    {
        if (!path.empty() && path[0] == '~')
        {
            const char *home = std::getenv("HOME");
            if (home)
            {
                return std::string(home) + path.substr(1);
            }
        }
        return path;
    }
}
