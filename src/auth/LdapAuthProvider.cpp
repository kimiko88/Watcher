#include "cms/auth/LdapAuthProvider.h"
#include <iostream>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <winldap.h>
#else
// Placeholder for non-windows (e.g. openldap)
#endif

namespace cms {
namespace auth {

LdapAuthProvider::LdapAuthProvider(const std::string &host, int port)
    : host_(host), port_(port) {}

LdapAuthProvider::~LdapAuthProvider() {}

bool LdapAuthProvider::authenticate(const std::string &username,
                                    const std::string &password,
                                    const std::string &domain) {
#ifdef _WIN32
  if (username.empty() || password.empty()) {
    return false;
  }

  // Initialize LDAP connection
  LDAP *ld = ldap_initA(const_cast<PCHAR>(host_.c_str()), port_);
  if (ld == NULL) {
    std::cerr << "ldap_init failed with " << LdapGetLastError() << std::endl;
    return false;
  }

  // Connect
  ULONG version = LDAP_VERSION3;
  ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, (void *)&version);

  ULONG connectStatus = ldap_connect(ld, NULL);
  if (connectStatus != LDAP_SUCCESS) {
    std::cerr << "ldap_connect failed: " << ldap_err2stringA(connectStatus)
              << std::endl;
    ldap_unbind(ld);
    return false;
  }

  // Construct DN or UPN
  std::string upn = username;
  if (!domain.empty()) {
    // Simple UPN format: username@domain
    // This generally works for Active Directory simple binds
    if (upn.find('@') == std::string::npos) {
      upn += "@" + domain;
    }
  }

  // Bind
  // ldap_simple_bind_s is synchronous
  // Note: sending password in clear text if not using SSL (ldaps)
  // For local AD testing it's usually fine, but in prod use SSL.
  ULONG bindStatus = ldap_simple_bind_sA(ld, const_cast<PCHAR>(upn.c_str()),
                                         const_cast<PCHAR>(password.c_str()));

  if (bindStatus == LDAP_SUCCESS) {
    ldap_unbind(ld);
    return true;
  } else {
    std::cerr << "Layout bind failed: " << ldap_err2stringA(bindStatus)
              << std::endl;
    ldap_unbind(ld);
    return false;
  }
#else
  std::cerr << "LDAP not supported on this platform" << std::endl;
  return false;
#endif
}

} // namespace auth
} // namespace cms
