# Minimum version of CMake required
cmake_minimum_required(VERSION 3.14)

# Project name and version
project(task_mollybet VERSION 1.0)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Enable all compiler warnings
add_compile_options(-Wall)

# Add the executable
add_executable(task src/main.cpp)

# Find and link the CURL, OpenSSL, and Crypto libraries
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)

target_link_libraries(task PRIVATE CURL::libcurl OpenSSL::SSL OpenSSL::Crypto)
