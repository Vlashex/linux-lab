cmake_minimum_required(VERSION 3.12)
project(kubsh)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(READLINE REQUIRED readline)
pkg_check_modules(FUSE REQUIRED fuse)

# Add executable
add_executable(kubsh main.cpp)

# Include directories
target_include_directories(kubsh PRIVATE
    ${READLINE_INCLUDE_DIRS}
    ${FUSE_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(kubsh
    ${READLINE_LIBRARIES}
    ${FUSE_LIBRARIES}
)

# Install target
install(TARGETS kubsh DESTINATION bin)