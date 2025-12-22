# IPC Tests
add_executable(ipc_tests
  test_ipc.cpp
)

target_link_libraries(ipc_tests
  PRIVATE
    cms_core
    gtest
    gtest_main
)

target_include_directories(ipc_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

# Add to CTest
add_test(NAME IPCTests COMMAND ipc_tests)
