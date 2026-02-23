cmake_minimum_required(VERSION 3.20)
project(mini_redis VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  add_compile_options(-O2)
endif()

# Static library shared between server binary and tests
add_library(mini_redis_lib STATIC
    src/resp.cpp
    src/store.cpp
    src/command.cpp
    src/server.cpp
    src/expiry.cpp
    src/aof.cpp
)
target_include_directories(mini_redis_lib PUBLIC src/)
target_link_libraries(mini_redis_lib PRIVATE pthread)

# Server binary
add_executable(mini_redis src/main.cpp)
target_link_libraries(mini_redis PRIVATE mini_redis_lib)

# Tests via googletest (FetchContent — no system install needed)
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_subdirectory(tests)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
