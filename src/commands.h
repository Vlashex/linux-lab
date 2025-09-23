#ifndef COMMANDS_H
#define COMMANDS_H

#include <string>
#include <vector>

namespace Commands
{
    // Возвращает true, если команда обработана встроенными средствами
    bool handleCommand(const std::vector<std::string> &args);
}

#endif // COMMANDS_H
