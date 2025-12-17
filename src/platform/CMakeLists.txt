# Platform-specific library

set(PLATFORM_SOURCES "")
set(PLATFORM_LIBS "")

# Select platform-specific source files
if(WIN32)
    message(STATUS "Building for Windows platform")
    list(APPEND PLATFORM_SOURCES WindowsPlatform.cpp)
    list(APPEND PLATFORM_SOURCES WindowsService.cpp)
    list(APPEND PLATFORM_SOURCES ScreenCaptureService.cpp)
    list(APPEND PLATFORM_SOURCES InputLockManager.cpp)
    list(APPEND PLATFORM_SOURCES Windows/ActivityMonitorWindows.cpp)
    # Windows-specific libraries
    set(PLATFORM_LIBS user32 gdi32 PowrProf Advapi32 pdh iphlpapi psapi) # Added networking/perf libs
elseif(APPLE)
    message(STATUS "Building for macOS platform")
    list(APPEND PLATFORM_SOURCES MacOSPlatform.cpp)
    # macOS-specific frameworks
    set(PLATFORM_LIBS
        "-framework CoreGraphics"
        "-framework IOKit"
        "-framework AppKit"
        "-framework Foundation"
    )
elseif(UNIX)
    message(STATUS "Building for Linux platform")
    list(APPEND PLATFORM_SOURCES LinuxPlatform.cpp)
    # Linux-specific libraries
    find_package(X11)
    if(X11_FOUND)
        set(PLATFORM_LIBS ${X11_LIBRARIES})
    endif()
endif()

# Create platform library
add_library(cms_platform STATIC ${PLATFORM_SOURCES})

target_include_directories(cms_platform
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(cms_platform
    PUBLIC
        cms_core
        ${PLATFORM_LIBS}
)

message(STATUS "Configured cms_platform library with sources: ${PLATFORM_SOURCES}")
