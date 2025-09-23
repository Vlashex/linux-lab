#include "vfs.h"

#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <string>
#include <thread>
#include <unistd.h>
#include <sys/inotify.h>
#include <vector>
#include <sys/wait.h>
#include <fstream>
#include <pwd.h>

namespace VFS
{
    static std::string getUsersDir()
    {
        const char *home = std::getenv("HOME");
        if (!home)
        {
            std::cerr << "kubsh: HOME not set" << std::endl;
            return "./users";
        }
        std::filesystem::path path(home);
        path /= "users";
        return path.string();
    }

    static bool runCommand(const std::vector<std::string> &args)
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            std::vector<char *> argv;
            for (const auto &a : args)
                argv.push_back(const_cast<char *>(a.c_str()));
            argv.push_back(nullptr);
            execvp(argv[0], argv.data());
            std::perror(("kubsh: exec " + args[0]).c_str());
            _exit(127);
        }
        else if (pid > 0)
        {
            int status = 0;
            waitpid(pid, &status, 0);
            return WIFEXITED(status) && WEXITSTATUS(status) == 0;
        }
        else
        {
            std::perror("kubsh: fork failed");
            return false;
        }
    }

    static bool addSystemUser(const std::string &name)
    {
        return runCommand({"adduser", "-D", "-s", "/bin/bash", name});
    }

    static bool delSystemUser(const std::string &name)
    {
        return runCommand({"deluser", name});
    }

    static void createUserFiles(const std::string &userDir, const std::string &username)
    {
        // гарантируем что каталог существует
        std::error_code ec;
        if (!std::filesystem::exists(userDir))
        {
            std::filesystem::create_directories(userDir, ec);
            if (ec)
            {
                std::cerr << "kubsh: cannot create dir " << userDir << ": " << ec.message() << std::endl;
                return;
            }
        }

        struct passwd *pwd = getpwnam(username.c_str());

        try
        {
            if (pwd)
            {
                std::ofstream(userDir + "/id") << pwd->pw_uid;
                std::ofstream(userDir + "/home") << pwd->pw_dir;
                std::ofstream(userDir + "/shell") << pwd->pw_shell;
            }
            else
            {
                // Заглушки
                std::ofstream(userDir + "/id") << -1;
                std::ofstream(userDir + "/home") << "UNKNOWN";
                std::ofstream(userDir + "/shell") << "UNKNOWN";
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "kubsh: failed to write user files: " << e.what() << std::endl;
        }
    }

    static void monitorUsersDir(const std::string &path)
    {
        int fd = inotify_init1(IN_NONBLOCK);
        if (fd == -1)
        {
            std::perror("kubsh: inotify_init1 failed");
            return;
        }

        int wd = inotify_add_watch(fd, path.c_str(), IN_CREATE | IN_DELETE);
        if (wd == -1)
        {
            std::perror("kubsh: inotify_add_watch failed");
            close(fd);
            return;
        }

        constexpr size_t buf_len = 4096;
        std::vector<char> buffer(buf_len);

        while (true)
        {
            ssize_t len = read(fd, buffer.data(), buffer.size());
            if (len <= 0)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                continue;
            }

            ssize_t i = 0;
            while (i < len)
            {
                struct inotify_event *event = reinterpret_cast<struct inotify_event *>(&buffer[i]);

                if (event->len > 0)
                {
                    std::string name(event->name);
                    std::string userDir = path + "/" + name;

                    if (event->mask & IN_CREATE)
                    {
                        std::cout << "[vfs] detected new folder: " << name << std::endl;

                        if (!addSystemUser(name))
                        {
                            std::cerr << "kubsh: failed to add user " << name << std::endl;
                        }

                        createUserFiles(userDir, name);
                    }
                    else if (event->mask & IN_DELETE)
                    {
                        std::cout << "[vfs] detected removed folder: " << name << std::endl;
                        delSystemUser(name);
                    }
                }

                i += sizeof(struct inotify_event) + event->len;
            }
        }

        close(fd);
    }

    void initUsers()
    {
        std::string dir = getUsersDir();

        try
        {
            if (!std::filesystem::exists(dir))
            {
                std::filesystem::create_directory(dir);
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "kubsh: failed to create users dir: " << e.what() << std::endl;
            return;
        }

        // Проверяем существующие каталоги пользователей
        for (auto &entry : std::filesystem::directory_iterator(dir))
        {
            if (entry.is_directory())
            {
                std::string name = entry.path().filename().string();
                createUserFiles(entry.path().string(), name);
            }
        }

        // Запускаем мониторинг в отдельном потоке
        std::thread monitorThread(monitorUsersDir, dir);
        monitorThread.detach();
    }

}
