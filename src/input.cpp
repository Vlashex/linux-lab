#include "input.h"
#include "history.h"

#include <iostream>
#include <termios.h>
#include <unistd.h>

namespace Input
{

    static void setRawMode(bool enable)
    {
        static struct termios oldt;
        static bool saved = false;

        if (enable)
        {
            if (!saved)
            {
                tcgetattr(STDIN_FILENO, &oldt);
                saved = true;
            }
            struct termios newt = oldt;
            newt.c_lflag &= ~(ICANON | ECHO);
            tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        }
        else
        {
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        }
    }

    std::string readline(const std::string &prompt)
    {
        std::cout << prompt << std::flush;

        std::string buffer;
        setRawMode(true);

        while (true)
        {
            char c;
            if (read(STDIN_FILENO, &c, 1) != 1)
                continue;

            if (c == '\n' || c == '\r')
            {
                std::cout << std::endl;
                break;
            }
            else if (c == 127 || c == '\b')
            { // backspace
                if (!buffer.empty())
                {
                    buffer.pop_back();
                    std::cout << "\b \b" << std::flush;
                }
            }
            else if (c == 27)
            { // escape sequence
                char seq[2];
                if (read(STDIN_FILENO, &seq[0], 1) != 1)
                    continue;
                if (read(STDIN_FILENO, &seq[1], 1) != 1)
                    continue;

                if (seq[0] == '[')
                {
                    if (seq[1] == 'A')
                    { // up arrow
                        std::string cmd = History::prev();
                        // стереть текущую строку
                        std::cout << "\33[2K\r" << prompt << cmd << std::flush;
                        buffer = cmd;
                    }
                    else if (seq[1] == 'B')
                    { // down arrow
                        std::string cmd = History::next();
                        std::cout << "\33[2K\r" << prompt << cmd << std::flush;
                        buffer = cmd;
                    }
                }
            }
            else if (c == 4)
            { // Ctrl+D
                setRawMode(false);
                std::cout << std::endl;
                return std::string(); // сигнал выхода
            }

            else
            {
                buffer.push_back(c);
                std::cout << c << std::flush;
            }
        }

        setRawMode(false);
        return buffer;
    }
}
