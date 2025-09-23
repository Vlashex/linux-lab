cmake_minimum_required(VERSION 3.10)

project(kubsh LANGUAGES CXX)

# Уровень стандарта
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Включаем строгие предупреждения
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -pedantic -Werror)
elseif (MSVC)
    add_compile_options(/W4 /WX)
endif()

# Источники проекта
set(SOURCES
    src/main.cpp
    src/executor.cpp
    src/history.cpp
    src/commands.cpp
    src/signals.cpp
    src/vfs.cpp
    src/utils.cpp
    src/input.cpp
)

set(HEADERS
    src/executor.h
    src/history.h
    src/commands.h
    src/signals.h
    src/vfs.h
    src/utils.h
    src/input.h
)

add_executable(kubsh ${SOURCES} ${HEADERS})
target_include_directories(kubsh PRIVATE ${CMAKE_SOURCE_DIR}/src)


# Устанавливаем бинарник в bin/
set_target_properties(kubsh PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
