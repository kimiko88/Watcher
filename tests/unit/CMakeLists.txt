# Unit Tests

# Collect all test sources
file(GLOB UNIT_TEST_SOURCES "*.cpp")

add_executable(cms_unit_tests ${UNIT_TEST_SOURCES})

target_link_libraries(cms_unit_tests PRIVATE
    cms_core
    cms_platform
    gtest
    gtest_main
)

target_include_directories(cms_unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

if (Qt6_FOUND)
    target_link_libraries(cms_unit_tests PRIVATE 
        Qt6::Core 
        Qt6::Network 
        Qt6::Widgets
    )
    if(WIN32)
        target_link_libraries(cms_unit_tests PRIVATE wldap32)
    endif()
endif()

if(WIN32)
    target_compile_definitions(cms_unit_tests PRIVATE _WIN32_WINNT=0x0601)
endif()

add_test(NAME UnitTests COMMAND cms_unit_tests)
