# Find Qt6 - Required for Master Service (Networking)
find_package(Qt6 COMPONENTS Core Network QUIET)

# Master service executable
add_executable(cms_master_service
    master_service_main.cpp
    MasterService.cpp
)

# If Qt is found, we can compile the real MasterServer
if (Qt6_FOUND)
    target_sources(cms_master_service PRIVATE 
        MasterServer.cpp
        MasterActivityAggregator.cpp
    )
    
    # Enable MOC for MasterServer which uses Q_OBJECT
    set_property(TARGET cms_master_service PROPERTY AUTOMOC ON)
    
    target_link_libraries(cms_master_service PRIVATE
        cms_core
        cms_platform
        Qt6::Core
        Qt6::Network
    )
else()
    message(WARNING "Qt6 not found! cms_master_service will fail to link createMasterServer unless a stub is provided.")
    target_link_libraries(cms_master_service PRIVATE
        cms_core
        cms_platform
    )
endif()

message(STATUS "Configured cms_master_service executable (lightweight, no MasterServer)")

if (Qt6_FOUND)
    message(STATUS "Qt6 found. Building cms_master GUI.")
    
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)

    # Master sources
    # Explicitly generate MOCs to solve linker errors
    set(MOC_HEADERS
        ${CMAKE_SOURCE_DIR}/include/cms/ui/ClientListModel.h
        ${CMAKE_SOURCE_DIR}/include/cms/ui/ClientThumbnailWidget.h
        ${CMAKE_SOURCE_DIR}/include/cms/ui/MasterWindow.h
        ${CMAKE_SOURCE_DIR}/include/cms/ui/RemoteViewWindow.h
        ${CMAKE_SOURCE_DIR}/include/cms/ui/RecordingPlayerWidget.h
        ${CMAKE_SOURCE_DIR}/include/cms/ui/RecordingPlayerWidget.h
        ${CMAKE_SOURCE_DIR}/include/cms/ui/ScreenshotViewerDialog.h
        ${CMAKE_SOURCE_DIR}/include/cms/ui/DomainPolicyDialog.h
        ${CMAKE_SOURCE_DIR}/include/cms/ui/ApplicationPolicyDialog.h
    )
    # Ensure Core is found for qt6_wrap_cpp
    find_package(Qt6 COMPONENTS Core REQUIRED)
    qt6_wrap_cpp(MOC_SOURCES ${MOC_HEADERS})

    set(MASTER_SOURCES
        MasterServer.cpp
        MasterActivityAggregator.cpp
        ui/ClientListModel.cpp
        ui/ClientThumbnailWidget.cpp
        ui/MasterWindow.cpp
        ui/RemoteViewWindow.cpp
        ui/RecordingPlayerWidget.cpp
        ui/RecordingPlayerWidget.cpp
        ui/ScreenshotViewerDialog.cpp
        ui/DomainPolicyDialog.cpp
        ui/ApplicationPolicyDialog.cpp
        main.cpp
        ${MOC_SOURCES}
    )

    # Application executable
    add_executable(cms_master ${MASTER_SOURCES})

    target_include_directories(cms_master PRIVATE 
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(cms_master PRIVATE 
        cms_core
        Qt6::Widgets
        Qt6::Network
    )
    
    # Link FFmpeg if available for RecordingPlayerWidget
    if(FFMPEG_FOUND)
        target_include_directories(cms_master PRIVATE ${FFMPEG_INCLUDE_DIRS})
        target_link_libraries(cms_master PRIVATE ${FFMPEG_LIBRARIES})
    endif()
    
    message(STATUS "Configured cms_master executable")
else()
    message(WARNING "Qt6 NOT found. Skipping cms_master GUI build. Please install Qt6 and set CMAKE_PREFIX_PATH to build the Master application.")
endif()
