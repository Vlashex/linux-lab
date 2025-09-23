#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <string>
#include <vector>

namespace Executor
{
    // Запуск внешней команды через fork + execvp
    // Возвращает код возврата дочернего процесса или -1 при ошибке fork/exec
    int runExternal(const std::vector<std::string> &args);
}

#endif // EXECUTOR_H
