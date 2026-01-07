#include "cms/auth/IAuthProvider.h"
#include "cms/auth/LdapAuthProvider.h"
#include <gtest/gtest.h>


// Stub for testing IAuthProvider interface
class StubAuthProvider : public cms::auth::IAuthProvider {
public:
  bool shouldSucceed = true;

  bool authenticate(const std::string &username, const std::string &password,
                    const std::string &domain) override {
    return shouldSucceed;
  }
};

TEST(AuthTest, InterfaceUsage) {
  StubAuthProvider auth;
  auth.shouldSucceed = true;
  EXPECT_TRUE(auth.authenticate("user", "pass"));

  auth.shouldSucceed = false;
  EXPECT_FALSE(auth.authenticate("user", "pass"));
}

TEST(AuthTest, LdapProviderInstantiation) {
  // Just verify we can instantiate it and it doesn't crash
  cms::auth::LdapAuthProvider ldap("localhost");
  // We expect authenticate to fail against localhost (or return false) without
  // crashing But in unit test environment, network calls might be problematic
  // or slow. So we might skip calling authenticate() or validat it returns
  // false gracefully.
  EXPECT_FALSE(ldap.authenticate("", "")); // Empty creds should fail fast
}
