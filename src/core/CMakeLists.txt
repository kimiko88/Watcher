# Core library sources
set(CORE_SOURCES
    Logger.cpp
    NetworkFilterManager.cpp
    RemoteControl.cpp
    Protocol.cpp
    Core.cpp
)

# Core library
add_library(cms_core STATIC
    ${CORE_SOURCES}
)

target_include_directories(cms_core
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

# Link threading library (required for Logger mutex)
find_package(Threads REQUIRED)
target_link_libraries(cms_core
    PUBLIC
        Threads::Threads
        nlohmann_json::nlohmann_json
)

message(STATUS "Configured cms_core library")
