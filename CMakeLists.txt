cmake_minimum_required(VERSION 3.14)
project(ClassroomControl VERSION 1.0.0 LANGUAGES CXX)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Platform detection
if(WIN32)
    add_compile_definitions(CMS_PLATFORM_WINDOWS)
    message(STATUS "Building for Windows")
elseif(APPLE)
    add_compile_definitions(CMS_PLATFORM_MACOS)
    message(STATUS "Building for macOS")
elseif(UNIX)
    add_compile_definitions(CMS_PLATFORM_LINUX)
    message(STATUS "Building for Linux")
endif()

# Build type specific flags
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Compiler flags for Debug and Release
if(MSVC)
    # MSVC compiler flags - use variables directly
    set(CMAKE_CXX_FLAGS_DEBUG "/W4 /Od /Zi /MDd /Zc:__cplusplus /permissive-")
    set(CMAKE_CXX_FLAGS_RELEASE "/W3 /O2 /MD /Zc:__cplusplus /permissive-")
else()
    # GCC/Clang compiler flags
    set(CMAKE_CXX_FLAGS_DEBUG "-Wall -Wextra -g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Google Test integration
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# nlohmann/json library for Protocol
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

# Enable testing
enable_testing()

# Add subdirectories
add_subdirectory(src/core)
add_subdirectory(src/platform)
add_subdirectory(src/client)
add_subdirectory(src/master)
add_subdirectory(tests)

# Print build summary
message(STATUS "")
message(STATUS "=== Classroom Control (CMS) Build Configuration ===")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "====================================================")
message(STATUS "")
