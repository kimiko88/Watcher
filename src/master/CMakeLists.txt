# Find Qt6
find_package(Qt6 COMPONENTS Widgets Network QUIET)

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
    )
    # Ensure Core is found for qt6_wrap_cpp
    find_package(Qt6 COMPONENTS Core REQUIRED)
    qt6_wrap_cpp(MOC_SOURCES ${MOC_HEADERS})

    set(MASTER_SOURCES
        MasterServer.cpp
        ui/ClientListModel.cpp
        ui/ClientThumbnailWidget.cpp
        ui/MasterWindow.cpp
        ui/RemoteViewWindow.cpp
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
    
    message(STATUS "Configured cms_master executable")
else()
    message(WARNING "Qt6 NOT found. Skipping cms_master GUI build. Please install Qt6 and set CMAKE_PREFIX_PATH to build the Master application.")
endif()
