#include "commands.h"

#include <iostream>
#include <vector>
#include <string>
#include <cstdlib> // getenv
#include <sstream>
#include <fstream>
#include <iterator>
#include <unistd.h> // chdir, access
#include <sys/types.h>
#include <sys/wait.h>
#include <filesystem>

namespace Commands
{

    // ===== helpers =====
    static std::string expandPath(const std::string &path)
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

    // ===== commands =====
    static void cmdEcho(const std::vector<std::string> &args)
    {
        for (std::size_t i = 1; i < args.size(); ++i)
        {
            std::cout << args[i];
            if (i + 1 < args.size())
            {
                std::cout << " ";
            }
        }
        std::cout << std::endl;
    }

    static void cmdEnv(const std::vector<std::string> &args)
    {
        if (args.size() < 2)
        {
            std::cerr << "kubsh: \\e requires variable name" << std::endl;
            return;
        }

        const std::string &varName = args[1];
        const char *value = std::getenv(varName.c_str());
        if (!value)
        {
            std::cerr << "kubsh: environment variable not found: " << varName << std::endl;
            return;
        }

        std::string valStr(value);
        std::istringstream ss(valStr);
        std::string token;

        if (valStr.find(':') != std::string::npos)
        {
            while (std::getline(ss, token, ':'))
            {
                std::cout << token << std::endl;
            }
        }
        else
        {
            std::cout << valStr << std::endl;
        }
    }

    static void cmdListPartitions(const std::vector<std::string> &args)
    {
        if (args.size() < 2)
        {
            std::cerr << "kubsh: \\l requires device path" << std::endl;
            return;
        }

        std::ifstream procPartitions("/proc/partitions");
        if (procPartitions.is_open())
        {
            std::string line;
            while (std::getline(procPartitions, line))
            {
                if (line.find(args[1]) != std::string::npos)
                {
                    std::cout << line << std::endl;
                }
            }
            return;
        }

        // fallback → lsblk
        pid_t pid = fork();
        if (pid == 0)
        {
            execlp("lsblk", "lsblk", args[1].c_str(), "-o", "NAME,SIZE,TYPE,MOUNTPOINT", nullptr);
            std::perror("kubsh: lsblk failed");
            _exit(127);
        }
        else if (pid > 0)
        {
            int status = 0;
            waitpid(pid, &status, 0);
        }
        else
        {
            std::perror("kubsh: fork failed");
        }
    }

    static void cmdCd(const std::vector<std::string> &args)
    {
        std::string target;
        if (args.size() < 2)
        {
            const char *home = std::getenv("HOME");
            if (!home)
            {
                std::cerr << "kubsh: cd: HOME not set" << std::endl;
                return;
            }
            target = home;
        }
        else
        {
            target = expandPath(args[1]);
        }

        if (chdir(target.c_str()) != 0)
        {
            std::perror("kubsh: cd");
        }
    }

    // ===== dispatcher =====
    bool handleCommand(const std::vector<std::string> &args)
    {
        if (args.empty())
        {
            return false;
        }

        const std::string &cmd = args[0];

        if (cmd == "\\q")
        {
            // Выход → main.cpp обработает break
            return true;
        }

        if (cmd == "echo")
        {
            cmdEcho(args);
            return true;
        }

        if (cmd == "\\e")
        {
            cmdEnv(args);
            return true;
        }

        if (cmd == "\\l")
        {
            cmdListPartitions(args);
            return true;
        }

        if (cmd == "cd")
        {
            cmdCd(args);
            return true;
        }

        return false; // не встроенная команда
    }

}
