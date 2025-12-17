# Client executable
add_executable(cms_client
    main.cpp
    ClientService.cpp
)

target_link_libraries(cms_client
    PRIVATE
        cms_core
        cms_platform
)

message(STATUS "Configured cms_client executable")
