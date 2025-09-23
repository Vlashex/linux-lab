#include "history.h"

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <filesystem>
#include <vector>

namespace History
{

    static std::vector<std::string> buffer;
    static int index = -1;

    static std::string getHistoryFile()
    {
        const char *home = std::getenv("HOME");
        if (!home)
        {
            return ".kubsh_history";
        }
        std::filesystem::path path(home);
        path /= ".kubsh_history";
        return path.string();
    }

    void load()
    {
        std::ifstream infile(getHistoryFile());
        if (!infile.is_open())
            return;

        std::string line;
        while (std::getline(infile, line))
        {
            if (!line.empty())
                buffer.push_back(line);
        }
        index = buffer.size(); // курсор в конец
    }

    const std::string &prev()
    {
        static std::string empty;
        if (buffer.empty())
            return empty;

        if (index > 0)
        {
            index--;
        }
        return buffer[index];
    }

    const std::string &next()
    {
        static std::string empty;
        if (buffer.empty())
            return empty;

        if (index < (int)buffer.size() - 1)
        {
            index++;
            return buffer[index];
        }
        else
        {
            index = buffer.size(); // вернуться к "новой строке"
            return empty;
        }
    }

    void append(const std::string &cmd)
    {
        if (cmd.empty())
            return;
        buffer.push_back(cmd);
        index = buffer.size();

        std::ofstream outfile(getHistoryFile(), std::ios::app);
        if (outfile.is_open())
        {
            outfile << cmd << std::endl;
        }
    }

    const std::vector<std::string> &all()
    {
        return buffer;
    }
}
