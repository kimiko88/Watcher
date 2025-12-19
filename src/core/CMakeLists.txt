# Core library sources
set(CORE_SOURCES
    Logger.cpp
    NetworkFilterManager.cpp
    RemoteControl.cpp
    Protocol.cpp
    Core.cpp
    Socket.cpp
    ScreenRecorder.cpp
    DomainPolicy.cpp
    ../network/Network.cpp
)

# Core library
add_library(cms_core STATIC
    ${CORE_SOURCES}
)

target_include_directories(cms_core
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

# Link FFmpeg if available
if(FFMPEG_FOUND)
    target_include_directories(cms_core PRIVATE ${FFMPEG_INCLUDE_DIRS})
    target_link_libraries(cms_core PRIVATE ${FFMPEG_LIBRARIES})
endif()

# Link threading library (required for Logger mutex)
find_package(Threads REQUIRED)
target_link_libraries(cms_core
    PUBLIC
        Threads::Threads
        nlohmann_json::nlohmann_json
)

if(WIN32)
    target_link_libraries(cms_core PRIVATE ws2_32 gdi32)
endif()

message(STATUS "Configured cms_core library")
