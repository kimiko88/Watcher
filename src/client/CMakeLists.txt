# Client service executable (runs as Windows service, spawns worker)
add_executable(cms_client_service
    client_service_main.cpp
    ServiceLauncher.cpp
)

target_link_libraries(cms_client_service
    PRIVATE
        cms_core
        cms_platform
        wtsapi32
        userenv
)

# Client worker executable (spawned by service, connects via IPC)
add_executable(cms_client_worker
    main.cpp
    ClientService.cpp
)

target_link_libraries(cms_client_worker
    PRIVATE
        cms_core
        cms_platform
)

message(STATUS "Configured cms_client_service and cms_client_worker executables")
