#include "signals.h"

#include <csignal>
#include <iostream>

namespace Signals
{

    // Обработчик сигналов
    void handler(int signum)
    {
        if (signum == SIGHUP)
        {
            std::cout << "Configuration reloaded" << std::endl;
        }
    }

    void setup()
    {
        struct sigaction sa{};
        sa.sa_handler = handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;

        if (sigaction(SIGHUP, &sa, nullptr) == -1)
        {
            std::perror("kubsh: sigaction failed");
        }
    }

}
