#ifndef CMS_AUTH_LDAP_AUTH_PROVIDER_H
#define CMS_AUTH_LDAP_AUTH_PROVIDER_H

#include "cms/auth/IAuthProvider.h"
#include <string>

namespace cms {
namespace auth {

class LdapAuthProvider : public IAuthProvider {
public:
  LdapAuthProvider(const std::string &host, int port = 389);
  ~LdapAuthProvider() override;

  bool authenticate(const std::string &username, const std::string &password,
                    const std::string &domain = "") override;

private:
  std::string host_;
  int port_;
};

} // namespace auth
} // namespace cms

#endif // CMS_AUTH_LDAP_AUTH_PROVIDER_H
