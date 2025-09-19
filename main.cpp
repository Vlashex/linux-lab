#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pwd.h>
#include <fuse.h>
#include <cerrno>

namespace fs = std::filesystem;

// Global variables
const char *history_path = nullptr;
bool running = true;

// FUSE-related variables
static struct fuse_operations vfs_ops;
std::string users_dir;

// Utility functions
void trim(std::string &str)
{
    str.erase(str.find_last_not_of(" \t\n\r\f\v") + 1);
    str.erase(0, str.find_first_not_of(" \t\n\r\f\v"));
}

std::vector<std::string> split_args(const std::string &input)
{
    std::vector<std::string> args;
    bool in_quote = false;
    std::string arg;

    for (char c : input)
    {
        if (c == '"')
        {
            in_quote = !in_quote;
        }
        else if (std::isspace(c) && !in_quote)
        {
            if (!arg.empty())
            {
                args.push_back(arg);
                arg.clear();
            }
        }
        else
        {
            arg += c;
        }
    }

    if (!arg.empty())
    {
        args.push_back(arg);
    }

    return args;
}

bool is_builtin(const std::string &cmd)
{
    return cmd == "\\q" || cmd == "echo" || cmd == "\\e" || cmd == "\\l";
}

// History functions
void init_history()
{
    const char *home = std::getenv("HOME");
    if (!home)
    {
        std::cerr << "Could not determine home directory" << std::endl;
        return;
    }

    std::string path = std::string(home) + "/.kubsh_history";
    history_path = strdup(path.c_str());

    if (fs::exists(history_path))
    {
        if (read_history(history_path) != 0)
        {
            std::cerr << "Could not read history file" << std::endl;
        }
    }
}

void save_history()
{
    if (history_path)
    {
        if (write_history(history_path) != 0)
        {
            std::cerr << "Could not write history file" << std::endl;
        }
        free((void *)history_path);
    }
}

// Signal handler
void sig_handler(int sig)
{
    const char *msg = "Configuration reloaded\n";
    write(STDOUT_FILENO, msg, strlen(msg));
}

// Built-in command handlers
void handle_echo(const std::vector<std::string> &args)
{
    for (size_t i = 1; i < args.size(); ++i)
    {
        std::cout << args[i];
        if (i < args.size() - 1)
        {
            std::cout << " ";
        }
    }
    std::cout << std::endl;
}

void handle_env(const std::vector<std::string> &args)
{
    if (args.size() < 2)
    {
        std::cerr << "Usage: \\e <env_var>" << std::endl;
        return;
    }

    const char *value = std::getenv(args[1].c_str());
    if (!value)
    {
        std::cerr << "Environment variable not found: " << args[1] << std::endl;
        return;
    }

    std::string val_str(value);
    if (val_str.find(':') != std::string::npos)
    {
        size_t start = 0;
        size_t end = val_str.find(':');
        while (end != std::string::npos)
        {
            std::cout << val_str.substr(start, end - start) << std::endl;
            start = end + 1;
            end = val_str.find(':', start);
        }
        std::cout << val_str.substr(start) << std::endl;
    }
    else
    {
        std::cout << val_str << std::endl;
    }
}

void handle_disk_info(const std::vector<std::string> &args)
{
    if (args.size() < 2)
    {
        std::cerr << "Usage: \\l <device>" << std::endl;
        return;
    }

    std::string cmd = "fdisk -l " + args[1];
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe)
    {
        std::cerr << "Error executing fdisk" << std::endl;
        return;
    }

    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    {
        std::cout << buffer;
    }

    pclose(pipe);
}

void execute_external(const std::vector<std::string> &args)
{
    std::vector<char *> c_args;
    for (const auto &arg : args)
    {
        c_args.push_back(const_cast<char *>(arg.c_str()));
    }
    c_args.push_back(nullptr);

    pid_t pid = fork();
    if (pid == 0)
    {
        // Child process
        execvp(c_args[0], c_args.data());
        std::cerr << "Error executing command: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    else if (pid > 0)
    {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
    }
    else
    {
        std::cerr << "Fork failed: " << strerror(errno) << std::endl;
    }
}

// FUSE functions
static int vfs_getattr(const char *path, struct stat *stbuf)
{
    int res = 0;
    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0)
    {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    }
    else
    {
        // Check if path corresponds to a user directory
        std::string path_str(path);
        if (path_str.find('/') != std::string::npos)
        {
            std::string username = path_str.substr(1);
            struct passwd *pwd = getpwnam(username.c_str());
            if (pwd)
            {
                stbuf->st_mode = S_IFDIR | 0755;
                stbuf->st_nlink = 2;
                stbuf->st_uid = pwd->pw_uid;
                stbuf->st_gid = pwd->pw_gid;
            }
            else
            {
                res = -ENOENT;
            }
        }
        else
        {
            res = -ENOENT;
        }
    }

    return res;
}

static int vfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi)
{
    (void)offset;
    (void)fi;

    if (strcmp(path, "/") != 0)
    {
        return -ENOENT;
    }

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    // List all users
    setpwent();
    struct passwd *pwd;
    while ((pwd = getpwent()) != NULL)
    {
        filler(buf, pwd->pw_name, NULL, 0);
    }
    endpwent();

    return 0;
}

static int vfs_mkdir(const char *path, mode_t mode)
{
    std::string path_str(path);
    if (path_str == "/")
    {
        return -EINVAL;
    }

    std::string username = path_str.substr(1);

    // Create the user using system command
    std::string cmd = "sudo adduser --system --no-create-home " + username;
    int result = system(cmd.c_str());

    if (result != 0)
    {
        return -EIO;
    }

    return 0;
}

static int vfs_rmdir(const char *path)
{
    std::string path_str(path);
    if (path_str == "/")
    {
        return -EINVAL;
    }

    std::string username = path_str.substr(1);

    // Delete the user using system command
    std::string cmd = "sudo userdel " + username;
    int result = system(cmd.c_str());

    if (result != 0)
    {
        return -EIO;
    }

    return 0;
}

void init_fuse_operations()
{
    vfs_ops.getattr = vfs_getattr;
    vfs_ops.readdir = vfs_readdir;
    vfs_ops.mkdir = vfs_mkdir;
    vfs_ops.rmdir = vfs_rmdir;
}

void start_vfs()
{
    // Create users directory if it doesn't exist
    const char *home = std::getenv("HOME");
    if (!home)
    {
        std::cerr << "Could not determine home directory for VFS" << std::endl;
        return;
    }

    users_dir = std::string(home) + "/users";
    if (mkdir(users_dir.c_str(), 0755) != 0 && errno != EEXIST)
    {
        std::cerr << "Could not create users directory: " << strerror(errno) << std::endl;
        return;
    }

    // Fork to run FUSE in background
    pid_t pid = fork();
    if (pid == 0)
    {
        // Child process - run FUSE
        char *args[] = {strdup("kubsh"), strdup("-f"), strdup(users_dir.c_str()), NULL};
        fuse_main(3, args, &vfs_ops, NULL);
        exit(0);
    }
    else if (pid < 0)
    {
        std::cerr << "Failed to start VFS: " << strerror(errno) << std::endl;
    }
}

int main()
{
    // Initialize signal handler
    struct sigaction sa;
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGHUP, &sa, NULL);

    // Initialize history
    using_history();
    init_history();

    // Initialize FUSE operations and start VFS
    init_fuse_operations();
    start_vfs();

    // Main shell loop
    char *input;
    while (running)
    {
        input = readline("kubsh> ");
        if (!input)
        {
            if (errno == EAGAIN)
                continue;
            std::cout << "\nExiting..." << std::endl;
            break;
        }

        std::string cmd_str(input);
        free(input);

        if (cmd_str.empty())
            continue;

        add_history(cmd_str.c_str());
        auto args = split_args(cmd_str);
        if (args.empty())
            continue;

        std::string cmd = args[0];

        if (cmd == "\\q")
        {
            running = false;
        }
        else if (cmd == "echo")
        {
            handle_echo(args);
        }
        else if (cmd == "\\e")
        {
            handle_env(args);
        }
        else if (cmd == "\\l")
        {
            handle_disk_info(args);
        }
        else if (is_builtin(cmd))
        {
            std::cout << cmd_str << std::endl;
        }
        else
        {
            execute_external(args);
        }
    }

    // Cleanup
    save_history();
    return 0;
}