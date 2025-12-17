# Network Filtering Implementation - Documentation

## Overview

The Network Filter feature allows blocking and allowing domains on Windows using the system **hosts file**. This provides a simple yet effective method for domain-level filtering without requiring complex firewall API integration.

## Implementation

### Approach

Instead of using the Windows Firewall COM API (INetFwPolicy2), which requires significant boilerplate code and COM initialization, we use the Windows hosts file located at:

```
C:\Windows\System32\drivers\etc\hosts
```

### How It Works

**Domain Blocking:**
1. Reads the existing hosts file
2. Appends a marker comment: `# CMS Blocked Domains`
3. Adds entries redirecting domains to `127.0.0.1` (localhost)
4. Flushes DNS cache using `ipconfig /flushdns`

**Domain Allowing:**
1. Reads the hosts file
2. Removes lines containing the domain and `127.0.0.1`
3. Rewrites the hosts file without those entries
4. Flushes DNS cache

**Example hosts file after blocking:**
```
# Copyright (c) 1993-2009 Microsoft Corp.
...
# CMS Blocked Domains
127.0.0.1 facebook.com
127.0.0.1 youtube.com
127.0.0.1 twitter.com
```

## Usage

### Blocking Domains

```cpp
auto platform = getPlatformInstance();

std::vector<std::string> domainsToBlock = {
    "facebook.com",
    "youtube.com",
    "instagram.com"
};

bool success = platform->blockDomains(domainsToBlock);
if (success) {
    LOG_INFO("Domains blocked successfully");
} else {
    LOG_ERROR("Failed to block domains (admin required)");
}
```

### Allowing Domains

```cpp
std::vector<std::string> domainsToAllow = {
    "facebook.com"
};

bool success = platform->allowDomains(domainsToAllow);
if (success) {
    LOG_INFO("Domains allowed successfully");
}
```

### Getting Current Rules

```cpp
auto rules = platform->getCurrentRules();

for (const auto& rule : rules) {
    LOG_INFO("Domain: " + rule.domain + 
             " | Blocked: " + (rule.blocked ? "Yes" : "No") +
             " | Reason: " + rule.reason);
}
```

## Security & Permissions

⚠️ **Administrator Privileges Required**

Modifying the hosts file requires administrator privileges on Windows. The application must be run as Administrator, or UAC will prompt for elevation.

**Permission Check:**
- If the hosts file cannot be opened for writing, the operation fails
- Error message logged: "Failed to open hosts file for writing (admin required)"

## Advantages

✅ **Simple** - No COM initialization or complex API calls
✅ **Effective** - Works at DNS resolution level
✅ **System-wide** - Affects all applications
✅ **Persistent** - Survives reboots
✅ **No dependencies** - Uses standard C++ file I/O

## Limitations

⚠️ **IP Address Bypass** - Users can access sites by IP address
⚠️ **DNS Cache** - Changes may take time to propagate
⚠️ **Manual Editing** - Users with admin can edit hosts file
⚠️ **No Allow List Mode** - Only block list supported
⚠️ **Domain Level Only** - Cannot block specific URLs/paths

## Comparison with Windows Firewall API

| Feature | Hosts File | Windows Firewall API |
|---------|-----------|---------------------|
| Complexity | Low | High (COM required) |
| Code | ~200 lines | ~1000+ lines |
| Admin Required | Yes | Yes |
| Domain Blocking | ✅ Yes | ✅ Yes |
| IP Blocking | ❌ No | ✅ Yes |
| Port Blocking | ❌ No | ✅ Yes |
| Allow List Mode | ❌ No | ✅ Yes |
| System-wide | ✅ Yes | ✅ Yes |

## Future Enhancements

### Short-term
- Wildcard domain support (`*.facebook.com`)
- Backup/restore hosts file
- Validation of domain names

### Medium-term
- Integration with Windows Firewall for IP blocking
- Process-specific filtering
- Time-based filtering

### Long-term
- Full Windows Firewall API implementation
- URL-level filtering (requires proxy)
- HTTPS inspection (requires certificate)

## Testing

The hosts file implementation is tested through the platform tests:

```cpp
// Test domain blocking
TEST_F(PlatformTest, BlockDomains) {
    auto platform = getPlatformInstance();
    
    std::vector<std::string> domains = {"test.com"};
    bool result = platform->blockDomains(domains);
    
    if (result) {
        // Verify domain is blocked
        auto rules = platform->getCurrentRules();
        bool found = false;
        for (const auto& rule : rules) {
            if (rule.domain == "test.com" && rule.blocked) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found);
        
        // Cleanup
        platform->allowDomains(domains);
    }
}
```

## Error Handling

All network filter operations include proper error handling:

```cpp
try {
    // File operations
} catch (const std::exception& e) {
    LOG_ERROR(std::string("Error blocking domains: ") + e.what());
    return false;
}
```

**Possible Errors:**
- File access denied (no admin privileges)
- File locked by another process
- Disk full
- Invalid domain names

## DNS Cache Flushing

After modifying the hosts file, the DNS cache is flushed to ensure changes take effect immediately:

```cpp
std::system("ipconfig /flushdns >nul 2>&1");
```

**Why Flush DNS:**
- Windows caches DNS lookups
- Cache may contain old IP addresses
- Flushing ensures blocked domains resolve to 127.0.0.1 immediately

## Platform-Specific Notes

### Windows
- **Path**: `C:\Windows\System32\drivers\etc\hosts`
- **Requires**: Administrator privileges
- **Format**: One entry per line: `IP domain`

### Linux (Future)
- **Path**: `/etc/hosts`
- **Requires**: Root/sudo
- **Format**: Same as Windows

### macOS (Future)
- **Path**: `/etc/hosts`
- **Requires**: Admin password
- **Format**: Same as Windows
- **Cache**: Use `dscacheutil -flushcache`

## Best Practices

1. **Always Check Return Values**
   ```cpp
   if (!platform->blockDomains(domains)) {
       // Handle failure
   }
   ```

2. **Cleanup on Exit**
   ```cpp
   // Unblock all domains when application closes
   auto rules = platform->getCurrentRules();
   std::vector<std::string> domains;
   for (const auto& rule : rules) {
       domains.push_back(rule.domain);
   }
   platform->allowDomains(domains);
   ```

3. **Log All Operations**
   ```cpp
   LOG_INFO("Blocking domain: example.com");
   bool result = platform->blockDomains({"example.com"});
   LOG_INFO(result ? "Success" : "Failed");
   ```

4. **Validate Domain Names**
   ```cpp
   bool isValidDomain(const std::string& domain) {
       // Check for valid characters, length, etc.
       return domain.find('.') != std::string::npos &&
              domain.length() < 255;
   }
   ```

## Troubleshooting

### Domains Not Blocked

**Problem**: Domains still accessible after blocking

**Solutions**:
1. Verify admin privileges
2. Check hosts file manually
3. Flush DNS cache manually: `ipconfig /flushdns`
4. Restart browser (may have cached connections)
5. Check if application is using DNS cache

### "Access Denied" Error

**Problem**: Cannot modify hosts file

**Solutions**:
1. Run application as Administrator
2. Check file permissions on hosts file
3. Disable antivirus temporarily (may lock file)
4. Check if another application is using the file

### Changes Not Taking Effect

**Problem**: DNS cache not clearing

**Solutions**:
1. Run `ipconfig /flushdns` manually
2. Restart browser
3. Clear browser cache
4. Wait a few minutes for TTL expiration

## Summary

The hosts file implementation provides a **simple, effective solution** for domain-level filtering on Windows. While it has limitations compared to a full firewall implementation, it's:

- ✅ Easy to implement and maintain
- ✅ No external dependencies
- ✅ System-wide effect
- ✅ Sufficient for classroom management
- ✅ Cross-platform compatible (concept applies to Linux/macOS too)

For more advanced filtering (IP addresses, ports, protocols), consider implementing the Windows Firewall API as a future enhancement.
