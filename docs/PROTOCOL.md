# Protocol Communication Layer Documentation

## Overview

The Protocol layer provides a robust JSON-based communication system for the Classroom Control Management System. It defines message structures, command types, serialization/deserialization, and integrity verification using CRC32 checksums.

## Architecture

```
┌─────────────────────────────────────────────────┐
│            Protocol Layer                       │
├─────────────────────────────────────────────────┤
│                                                 │
│  ┌─────────────┐         ┌──────────────┐     │
│  │   Message   │         │ CommandType  │     │
│  │   Struct    │         │    Enum      │     │
│  └──────┬──────┘         └──────────────┘     │
│         │                                      │
│  ┌──────▼──────────────────────────────┐      │
│  │     MessageSerializer                │      │
│  ├──────────────────────────────────────┤      │
│  │  - Serialize()   (Message → JSON)   │      │
│  │  - Deserialize() (JSON → Message)   │      │
│  │  - Validate()    (CRC32 check)      │      │
│  │  - CalculateCRC32()                 │      │
│  └─────────────────────────────────────┘      │
│                                                 │
└─────────────────────────────────────────────────┘
```

## Message Format

Every message is represented in JSON with three main sections:

### Structure

```json
{
  "header": {
    "message_id": "f47ac10b-58cc-4372-a567-0e02b2c3d479",
    "type": "SCREENSHOT_REQUEST",
    "timestamp": 1702806000,
    "source": "master",
    "destination": "client-001"
  },
  "payload": {
    "quality": 80,
    "format": "jpeg"
  },
  "checksum": "A3B5C7D9"
}
```

### Header Fields

| Field | Type | Description |
|-------|------|-------------|
| `message_id` | string | Unique UUID-like identifier |
| `type` | string | Command type (see CommandType enum) |
| `timestamp` | int64 | Unix timestamp (seconds since epoch) |
| `source` | string | Sender identifier |
| `destination` | string | Receiver identifier |

### Payload

The payload is a flexible JSON object containing command-specific data. Structure varies by command type.

### Checksum

CRC32 checksum (hex string) of the header + payload (calculated before adding checksum field). Used for integrity verification.

## Command Types

The system supports 12 command types:

| Command | Direction | Description | Example Payload |
|---------|-----------|-------------|-----------------|
| `HELLO` | Client → Master | Initial handshake | `{"version": "1.0.0", "capabilities": [...]}` |
| `SCREENSHOT_REQUEST` | Master → Client | Request screenshot | `{"quality": 80, "format": "jpeg"}` |
| `SCREENSHOT_DATA` | Client → Master | Screenshot response | `{"data": "base64...", "width": 1920, "height": 1080}` |
| `SCREEN_LOCK` | Master → Client | Lock screen | `{"message": "Screen locked by teacher"}` |
| `SCREEN_UNLOCK` | Master → Client | Unlock screen | `{}` |
| `SCREEN_BROADCAST` | Master → Client | Broadcast screen | `{"master_screen_id": "123"}` |
| `POWER_CONTROL` | Master → Client | Power management | `{"action": "shutdown", "delay_seconds": 30}` |
| `DOMAIN_BLOCK` | Master → Client | Block domain | `{"domain": "example.com", "reason": "..."}` |
| `DOMAIN_ALLOW` | Master → Client | Allow domain | `{"domain": "example.com"}` |
| `STATUS_UPDATE` | Client → Master | Status report | `{"cpu": 45.2, "memory": 60.5, "status": "active"}` |
| `PING` | Bidirectional | Keepalive | `{}` |
| `DISCONNECT` | Bidirectional | Disconnect notice | `{"reason": "user_logout"}` |

## Usage Examples

### Creating a Message

```cpp
#include "cms/Protocol.h"

using namespace cms::protocol;

// Create HELLO message
nlohmann::json payload = {
    {"version", "1.0.0"},
    {"capabilities", {"screenshot", "screen_lock"}},
    {"os", "Windows"}
};

auto msg = Message::Create(
    CommandType::HELLO,
    "client-001",
    "master",
    payload
);

// message_id and timestamp are auto-generated
```

### Serializing a Message

```cpp
MessageSerializer serializer;

auto json_str = serializer.Serialize(msg);
// Returns JSON string with calculated checksum

// Now send json_str over network...
```

### Deserializing a Message

```cpp
MessageSerializer serializer;

// Received from network
std::string json_str = "...";

try {
    auto msg = serializer.Deserialize(json_str);
    
    // Now you have a Message object
    std::cout << "Type: " << CommandTypeToString(msg.type) << std::endl;
    std::cout << "From: " << msg.source << std::endl;
    
} catch (const std::exception& e) {
    LOG_ERROR(std::string("Invalid message: ") + e.what());
}
```

### Validating a Message

```cpp
MessageSerializer serializer;

auto msg = serializer.Deserialize(json_str);

if (serializer.Validate(msg)) {
    LOG_INFO("Message integrity verified");
    // Process message
} else {
    LOG_ERROR("Message tampered! Checksum mismatch");
    // Reject message
}
```

## Complete Example: Screenshot Request Flow

```cpp
// MASTER: Request screenshot from client

MessageSerializer serializer;

// 1. Create request
nlohmann::json request_payload = {
    {"quality", 80},
    {"format", "jpeg"},
    {"width", 1920},
    {"height", 1080}
};

auto request = Message::Create(
    CommandType::SCREENSHOT_REQUEST,
    "master",
    "client-001",
    request_payload
);

// 2. Serialize
auto request_json = serializer.Serialize(request);

// 3. Send over network
network::Send(client_connection, request_json);

// ---

// CLIENT: Receive and process

// 4. Receive from network
std::string received_json = network::Receive();

// 5. Deserialize
auto received_request = serializer.Deserialize(received_json);

// 6. Validate
if (!serializer.Validate(received_request)) {
    LOG_ERROR("Invalid request received");
    return;
}

// 7. Process request
if (received_request.type == CommandType::SCREENSHOT_REQUEST) {
    int quality = received_request.payload["quality"];
    
    // Take screenshot...
    std::string screenshot_base64 = TakeScreenshot(quality);
    
    // 8. Create response
    nlohmann::json response_payload = {
        {"data", screenshot_base64},
        {"width", 1920},
        {"height", 1080},
        {"format", "jpeg"}
    };
    
    auto response = Message::Create(
        CommandType::SCREENSHOT_DATA,
        "client-001",
        "master",
        response_payload
    );
    
    // 9. Serialize response
    auto response_json = serializer.Serialize(response);
    
    // 10. Send back to master
    network::Send(master_connection, response_json);
}
```

## CRC32 Checksum

### Algorithm

The implementation uses the standard CRC32 algorithm with polynomial 0xEDB88320.

### Calculation Process

1. Convert Message to JSON (header + payload only, no checksum)
2. Calculate CRC32 of the JSON string
3. Convert CRC32 to 8-character uppercase hex string
4. Add checksum to final JSON

### Validation Process

1. Extract checksum from received message
2. Remove checksum field from message
3. Recalculate CRC32 from header + payload
4. Compare calculated with received checksum
5. Return true if match, false otherwise

## Error Handling

### Deserialization Errors

The `Deserialize()` method throws exceptions for:

- **Malformed JSON**: `nlohmann::json::parse_error`
- **Missing header**: `std::runtime_error`
- **Missing required field**: `std::runtime_error`
- **Invalid command type**: `std::invalid_argument`

### Best Practices

```cpp
try {
    auto msg = serializer.Deserialize(json_str);
    
    if (!serializer.Validate(msg)) {
        LOG_WARNING("Message checksum invalid");
        return;
    }
    
    // Process message
    
} catch (const std::invalid_argument& e) {
    LOG_ERROR(std::string("Invalid command: ") + e.what());
} catch (const std::runtime_error& e) {
    LOG_ERROR(std::string("Message error: ") + e.what());
} catch (const std::exception& e) {
    LOG_ERROR(std::string("Unexpected error: ") + e.what());
}
```

## Testing

The Protocol layer includes comprehensive TDD tests (40+ test cases):

### Test Categories

1. **Command Type Tests**
   - String to enum conversion
   - Enum to string conversion
   - Invalid type handling

2. **Message Creation Tests**
   - Manual message creation
   - Factory method creation
   - Auto-generated fields

3. **Utility Function Tests**
   - UUID generation uniqueness
   - Timestamp generation
   - Timestamp increment

4. **CRC32 Tests**
   - Calculation correctness
   - Deterministic behavior
   - Empty string handling

5. **Serialization Tests**
   - All command types
   - JSON structure verification
   - Required fields presence

6. **Deserialization Tests**
   - Valid JSON parsing
   - Field extraction
   - Checksum extraction

7. **Idempotency Tests**
   - Serialize → Deserialize round-trip
   - Data preservation

8. **Validation Tests**
   - Valid checksum acceptance
   - Invalid checksum rejection
   - Tamper detection

9. **Error Handling Tests**
   - Malformed JSON
   - Missing fields
   - Invalid types

10. **Realistic Scenario Tests**
    - HELLO handshake flow
    - Screenshot request/response
    - Domain blocking
    - Status updates

### Running Tests

```bash
# Build
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Run protocol tests only
./build/tests/cms_unit_tests --gtest_filter=ProtocolTest.*

# Run all tests
cd build
ctest --output-on-failure
```

## Performance Considerations

### Serialization

- **Time Complexity**: O(n) where n is payload size
- **Space Complexity**: O(n) for JSON string allocation
- **Typical Latency**: < 1ms for small payloads

### Deserialization

- **Time Complexity**: O(n) for JSON parsing
- **Space Complexity**: O(n) for Message object
- **Typical Latency**: < 1ms for small payloads

### CRC32 Calculation

- **Time Complexity**: O(n) where n is message size
- **Space Complexity**: O(1) constant
- **Typical Latency**: < 0.1ms for messages < 1KB

### Optimization Tips

1. **Reuse MessageSerializer**: Create once, use many times
2. **Payload Size**: Keep payloads small when possible
3. **Binary Data**: Use base64 encoding sparingly
4. **Caching**: Cache serialized messages if sending to multiple recipients

## Security Considerations

### Current Security Features

✅ **Integrity**: CRC32 checksum detects accidental corruption
✅ **Type Safety**: Strong typing prevents command confusion
✅ **Validation**: All fields are validated during deserialization

### Current Limitations

⚠️ **No Encryption**: Messages are plaintext JSON
⚠️ **No Authentication**: No sender verification
⚠️ **No Replay Protection**: No sequence numbers
⚠️ **Weak Checksum**: CRC32 is not cryptographically secure

### Recommended Enhancements

For production deployment:

1. **Transport Encryption**: Use TLS/SSL
2. **Message Authentication**: HMAC-SHA256 or digital signatures
3. **Sequence Numbers**: Prevent replay attacks
4. **Rate Limiting**: Prevent DoS attacks
5. **Access Control**: Role-based permissions

## Dependencies

- **nlohmann/json**: JSON parsing (v3.11.3)
  - Header-only library
  - Fetched automatically via CMake FetchContent
  - No runtime dependencies

## API Reference

### Enums

#### CommandType

```cpp
enum class CommandType {
    HELLO, SCREENSHOT_REQUEST, SCREENSHOT_DATA,
    SCREEN_LOCK, SCREEN_UNLOCK, SCREEN_BROADCAST,
    POWER_CONTROL, DOMAIN_BLOCK, DOMAIN_ALLOW,
    STATUS_UPDATE, PING, DISCONNECT
};
```

### Structs

#### Message

```cpp
struct Message {
    std::string message_id;
    CommandType type;
    int64_t timestamp;
    std::string source;
    std::string destination;
    nlohmann::json payload;
    std::string checksum;
    
    static Message Create(CommandType type, const std::string& source,
                         const std::string& destination,
                         const nlohmann::json& payload);
};
```

### Classes

#### MessageSerializer

```cpp
class MessageSerializer {
public:
    std::string Serialize(const Message& msg);
    Message Deserialize(const std::string& json_str);
    bool Validate(const Message& msg);
    uint32_t CalculateCRC32(const std::string& data);
};
```

### Functions

```cpp
std::string CommandTypeToString(CommandType type);
CommandType StringToCommandType(const std::string& str);
std::string GenerateMessageId();
int64_t GetCurrentTimestamp();
```

## Next Steps

1. **Network Layer Integration**: Connect Protocol to actual network I/O
2. **Message Queue**: Implement reliable message delivery
3. **Compression**: Add gzip compression for large payloads
4. **Binary Protocol**: Optionally support Protocol Buffers
5. **Security**: Add HMAC authentication and encryption
