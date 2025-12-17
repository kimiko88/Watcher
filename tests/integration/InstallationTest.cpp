#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>


namespace fs = std::filesystem;

class InstallationTest : public ::testing::Test {
protected:
  std::string project_root;

  void SetUp() override {
    // Try to locate project root by looking for "scripts" directory
    // Start from current directory and go up
    fs::path current = fs::current_path();

    // Simple heuristic: check up to 4 levels up
    for (int i = 0; i < 4; ++i) {
      if (fs::exists(current / "scripts")) {
        project_root = current.string() + "/";
        return;
      }
      if (current.has_parent_path()) {
        current = current.parent_path();
      } else {
        break;
      }
    }

    // Fallback: assume we are in build/tests/Debug, so root is ../../../
    project_root = "../../../";
  }
};

TEST_F(InstallationTest, DirectoriesExist) {
  if (project_root.empty()) {
    SUCCEED() << "Could not verify directories: project root not found";
    return;
  }
  EXPECT_TRUE(fs::exists(project_root + "scripts/windows"))
      << "Missing scripts/windows";
  EXPECT_TRUE(fs::exists(project_root + "scripts/linux"))
      << "Missing scripts/linux";
  EXPECT_TRUE(fs::exists(project_root + "scripts/macos"))
      << "Missing scripts/macos";
}

TEST_F(InstallationTest, WindowsScriptExists) {
  if (project_root.empty())
    return;
  std::string script_path = project_root + "scripts/windows/install_client.ps1";
  EXPECT_TRUE(fs::exists(script_path))
      << "Windows install script missing: " << script_path;
}

TEST_F(InstallationTest, LinuxScriptExists) {
  if (project_root.empty())
    return;
  std::string script_path = project_root + "scripts/linux/install_client.sh";
  EXPECT_TRUE(fs::exists(script_path))
      << "Linux install script missing: " << script_path;
}

TEST_F(InstallationTest, MacOsScriptExists) {
  if (project_root.empty())
    return;
  std::string script_path = project_root + "scripts/macos/install_client.sh";
  EXPECT_TRUE(fs::exists(script_path))
      << "macOS install script missing: " << script_path;
}

#ifdef _WIN32
TEST_F(InstallationTest, WindowsScriptSyntaxCheck) {
  if (project_root.empty())
    return;
  std::string script_path = project_root + "scripts/windows/install_client.ps1";
  if (!fs::exists(script_path))
    return;

  // Check strict error handling
  std::string cmd =
      "powershell -Command \"& { $err = $null; "
      "[System.Management.Automation.PSParser]::Tokenize((Get-Content '" +
      script_path +
      "' -Raw), [ref]$err) | Out-Null; if ($err) { exit 1 } else { exit 0 } "
      "}\"";

  int result = std::system(cmd.c_str());
  EXPECT_EQ(result, 0) << "Windows script syntax error";
}
#endif
