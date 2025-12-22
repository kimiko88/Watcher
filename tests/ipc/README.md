# IPC Tests

## Running Tests

```batch
cd build
cmake --build . --target ipc_tests --config Release
ctest -C Release --output-on-failure
```

## Test Coverage

### IPCProtocol Tests
- [x] Message creation
- [x] Message serialization/deserialization
- [x] Payload handling
- [x] Timestamp generation
- [x] ID uniqueness

### IPCChannel Tests
- [x] Named Pipe server creation
- [x] Named Pipe client connection
- [x] Basic message sending
- [x] Reconnection handling
- [x] Error handling

### Integration Tests
- [x] Server-client communication
- [x] Multiple clients
- [x] Heartbeat mechanism
- [x] Message ordering

## Adding New Tests

1. Create test file in `tests/ipc/test_*.cpp`
2. Use Google Test framework
3. Add to CMakeLists.txt
4. Run `ctest`

Example:
```cpp
TEST(IPCTest, YourTestName) {
  // Arrange
  NamedPipeServer server("TestPipe");
  
  // Act
  bool result = server.Start();
  
  // Assert
  EXPECT_TRUE(result);
  
  // Cleanup
  server.Stop();
}
```

## Known Issues

- Named Pipes are Windows-only
- Some tests may be flaky under high load
- Timing-dependent tests have generous timeouts

## TODO

- [ ] Add stress tests (many messages)
- [ ] Add failure scenario tests
- [ ] Add performance benchmarks
- [ ] Mock ServiceLauncher for unit testing
