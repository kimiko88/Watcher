#include "cms/Socket.h"

// Network layer placeholder
// This file will contain networking functionality in future iterations

namespace cms {
namespace network {

// Network initialization
void InitializeNetwork() { cms::Socket::Initialize(); }

// Network cleanup
void ShutdownNetwork() { cms::Socket::Cleanup(); }

} // namespace network
} // namespace cms
