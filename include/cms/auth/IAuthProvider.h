#ifndef CMS_AUTH_I_AUTH_PROVIDER_H
#define CMS_AUTH_I_AUTH_PROVIDER_H

#include <string>

namespace cms {
namespace auth {

class IAuthProvider {
public:
  virtual ~IAuthProvider() = default;

  /**
   * @brief Authenticate a user.
   * @param username The username to authenticate.
   * @param password The password.
   * @param domain Optional domain.
   * @return true if authentication is successful.
   */
  virtual bool authenticate(const std::string &username,
                            const std::string &password,
                            const std::string &domain = "") = 0;
};

} // namespace auth
} // namespace cms

#endif // CMS_AUTH_I_AUTH_PROVIDER_H
