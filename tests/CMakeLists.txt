# Test configuration
include(GoogleTest)

# Core Unit Tests Sources (Always build)
set(TEST_SOURCES
    unit/test_common.cpp
    unit/test_logger.cpp
    unit/test_protocol.cpp
    unit/test_platform.cpp
    unit/test_client_service.cpp
    unit/test_screen_capture.cpp
    unit/test_input_lock.cpp
    unit/test_network_filter.cpp
    unit/test_remote_control.cpp
    ${CMAKE_SOURCE_DIR}/src/client/ClientService.cpp
)

# Optional UI Tests Sources
find_package(Qt6 COMPONENTS Widgets Test QUIET)

    # Explicitly generate MOC for ClientListModel
    find_package(Qt6 COMPONENTS Core REQUIRED)
    qt6_wrap_cpp(TEST_MOC_SOURCES ${CMAKE_SOURCE_DIR}/include/cms/ui/ClientListModel.h)

    list(APPEND TEST_SOURCES
        unit/test_ui_model.cpp
        ${CMAKE_SOURCE_DIR}/src/master/ui/ClientListModel.cpp
        ${TEST_MOC_SOURCES}
    )

# Unit tests executable
add_executable(cms_unit_tests ${TEST_SOURCES})

# Basic link libraries
target_link_libraries(cms_unit_tests
    PRIVATE
        cms_core
        cms_platform
        GTest::gtest
        GTest::gtest_main
)

# Link Qt if available
if (Qt6_FOUND)
    target_link_libraries(cms_unit_tests
        PRIVATE
            Qt6::Widgets
            Qt6::Test
    )
    # Enable MOC for tests too
    set_target_properties(cms_unit_tests PROPERTIES AUTOMOC ON)
endif()

# Integration tests executable
add_executable(cms_integration_tests
    integration/test_build.cpp
)

target_link_libraries(cms_integration_tests
    PRIVATE
        cms_core
        GTest::gtest
        GTest::gtest_main
)

# Discover and register tests
gtest_discover_tests(cms_unit_tests)
gtest_discover_tests(cms_integration_tests)

message(STATUS "Configured CMS test suites")
message(STATUS "  - cms_unit_tests: Unit tests for individual modules")
message(STATUS "  - cms_integration_tests: Integration tests for build verification")
