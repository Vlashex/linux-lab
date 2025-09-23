#include <iostream>
#include <string>
#include <vector>
#include <csignal>

#include "utils.h"
#include "commands.h"
#include "executor.h"
#include "history.h"
#include "signals.h"
#include "vfs.h"
#include "input.h"

int main()
{
    // Инициализация подсистем
    History::load();
    Signals::setup();
    VFS::initUsers();

    std::string line;

    while (true)
    {
        std::string line = Input::readline("kubsh> ");
        if (line.empty())
        {
            // Пустая строка → возможно Ctrl+D
            if (std::cin.eof())
            {
                break;
            }
            continue;
        }

        std::vector<std::string> args = Utils::split(line);
        if (args.empty())
        {
            continue;
        }

        if (Commands::handleCommand(args))
        {
            History::append(line);
            if (args[0] == "\\q")
            {
                break;
            }
            continue;
        }

        int status = Executor::runExternal(args);
        if (status != 0)
        {
            std::cerr << "kubsh: command failed with code " << status << std::endl;
        }
        History::append(line);
    }

    return 0;
}
