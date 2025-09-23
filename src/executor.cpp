#include "executor.h"
#include "utils.h" // для expandTilde

#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstring>

namespace Executor
{

    int runExternal(const std::vector<std::string> &args)
    {
        if (args.empty())
        {
            return -1;
        }

        pid_t pid = fork();
        if (pid < 0)
        {
            std::perror("kubsh: fork failed");
            return -1;
        }

        if (pid == 0)
        {
            // Дочерний процесс
            std::vector<std::string> expanded;
            expanded.reserve(args.size());

            for (const auto &arg : args)
            {
                expanded.push_back(Utils::expandTilde(arg));
            }

            std::vector<char *> argv;
            argv.reserve(expanded.size() + 1);

            for (auto &arg : expanded)
            {
                argv.push_back(const_cast<char *>(arg.c_str()));
            }
            argv.push_back(nullptr);

            execvp(argv[0], argv.data());

            // Если execvp вернулся, значит произошла ошибка
            std::perror("kubsh: command not found");
            _exit(127);
        }

        // Родительский процесс ждёт завершения
        int status = 0;
        if (waitpid(pid, &status, 0) == -1)
        {
            std::perror("kubsh: waitpid failed");
            return -1;
        }

        if (WIFEXITED(status))
        {
            return WEXITSTATUS(status);
        }

        if (WIFSIGNALED(status))
        {
            std::cerr << "kubsh: terminated by signal " << WTERMSIG(status) << std::endl;
            return 128 + WTERMSIG(status);
        }

        return -1;
    }

}
