#ifndef INPUT_H
#define INPUT_H

#include <string>

namespace Input
{
    // Читает строку с поддержкой истории (стрелки ↑ и ↓)
    std::string readline(const std::string &prompt);
}

#endif // INPUT_H
