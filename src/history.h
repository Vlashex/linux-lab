#ifndef HISTORY_H
#define HISTORY_H

#include <string>
#include <vector>

namespace History
{
    void load();
    void append(const std::string &cmd);

    // Навигация по истории
    const std::string &prev();
    const std::string &next();

    // Внутренний буфер истории
    const std::vector<std::string> &all();
}

#endif // HISTORY_H
