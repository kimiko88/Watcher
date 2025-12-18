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

# FFmpeg library for video recording/playback
message(STATUS "Searching for FFmpeg libraries...")

# Try pkg-config first
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(FFMPEG QUIET
        libavcodec
        libavformat
        libavutil
        libswscale
        libavdevice
    )
endif()

# Manual search on Windows if pkg-config failed
if(NOT FFMPEG_FOUND AND WIN32)
    message(STATUS "pkg-config not found or FFmpeg not detected, trying manual search...")
    
    # Common FFmpeg installation paths on Windows
    set(FFMPEG_HINT_PATHS
        "C:/ffmpeg"
        "C:/Program Files/ffmpeg"
        "C:/Program Files (x86)/ffmpeg"
        "$ENV{FFMPEG_ROOT}"
        "$ENV{ProgramFiles}/ffmpeg"
    )
    
    find_path(FFMPEG_INCLUDE_DIR
        NAMES libavcodec/avcodec.h
        PATHS ${FFMPEG_HINT_PATHS}
        PATH_SUFFIXES include
    )
    
    find_library(AVCODEC_LIBRARY
        NAMES avcodec
        PATHS ${FFMPEG_HINT_PATHS}
        PATH_SUFFIXES lib bin
    )
    
    find_library(AVFORMAT_LIBRARY
        NAMES avformat
        PATHS ${FFMPEG_HINT_PATHS}
        PATH_SUFFIXES lib bin
    )
    
    find_library(AVUTIL_LIBRARY
        NAMES avutil
        PATHS ${FFMPEG_HINT_PATHS}
        PATH_SUFFIXES lib bin
    )
    
    find_library(SWSCALE_LIBRARY
        NAMES swscale
        PATHS ${FFMPEG_HINT_PATHS}
        PATH_SUFFIXES lib bin
    )
    
    find_library(AVDEVICE_LIBRARY
        NAMES avdevice
        PATHS ${FFMPEG_HINT_PATHS}
        PATH_SUFFIXES lib bin
    )
    
    if(FFMPEG_INCLUDE_DIR AND AVCODEC_LIBRARY AND AVFORMAT_LIBRARY AND AVUTIL_LIBRARY AND SWSCALE_LIBRARY)
        set(FFMPEG_FOUND TRUE)
        set(FFMPEG_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIR})
        set(FFMPEG_LIBRARIES ${AVCODEC_LIBRARY} ${AVFORMAT_LIBRARY} ${AVUTIL_LIBRARY} ${SWSCALE_LIBRARY})
        if(AVDEVICE_LIBRARY)
            list(APPEND FFMPEG_LIBRARIES ${AVDEVICE_LIBRARY})
        endif()
        message(STATUS "FFmpeg found manually at: ${FFMPEG_INCLUDE_DIR}")
    endif()
endif()

if(FFMPEG_FOUND)
    message(STATUS "FFmpeg libraries found:")
    message(STATUS "  Include dirs: ${FFMPEG_INCLUDE_DIRS}")
    message(STATUS "  Libraries: ${FFMPEG_LIBRARIES}")
    add_compile_definitions(CMS_HAS_FFMPEG)
else()
    message(WARNING "FFmpeg NOT found. Screen recording will use stub implementation.")
    message(WARNING "To enable FFmpeg:")
    message(WARNING "  - Install FFmpeg from https://www.gyan.dev/ffmpeg/builds/")
    message(WARNING "  - Set FFMPEG_ROOT environment variable to installation path")
    message(WARNING "  - Or install via vcpkg: vcpkg install ffmpeg")
endif()

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
