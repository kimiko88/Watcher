#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>


namespace fs = std::filesystem;

class InstallationTest : public ::testing::Test {
protected:
  std::string scripts_dir;

  void SetUp() override {
    // Assume scripts are in the root source directory "scripts" folder
    // For tests running in build/tests/Debug, we need to go up a few levels or
    // use a defined path We will assume CMAKE_SOURCE_DIR is passed or we can
    // find it relative to execution

    // Simple heuristic: search for scripts folder upwards
    auto current_path = fs::current_path();
    while (!fs::exists(current_path / "scripts") &&
           current_path.has_parent_path()) {
      current_path = current_path.parent_path();
    }

    if (fs::exists(current_path / "scripts")) {
      scripts_dir = (current_path / "scripts").string();
    } else {
      // Fallback for hardcoded common location if test is run from weird place
      // This might fail if project structure is very different, but standard
      // build is ok.
      scripts_dir = "../../scripts";
    }
  }
};

// ============================================================================
// WINDOWS TESTS
// ============================================================================

TEST_F(InstallationTest, WindowsScriptExists) {
  fs::path script_path =
      fs::path(scripts_dir) / "windows" / "install_client.ps1";
  EXPECT_TRUE(fs::exists(script_path))
      << "install_client.ps1 not found at " << script_path;
}

#ifdef _WIN32
TEST_F(InstallationTest, WindowsScriptSyntaxCheck) {
  fs::path script_path =
      fs::path(scripts_dir) / "windows" / "install_client.ps1";
  if (!fs::exists(script_path))
    return;

  std::string command = "powershell -Command \"Get-Command -Syntax '" +
                        script_path.string() + "'\"";
  int result = std::system(command.c_str());
  EXPECT_EQ(result, 0) << "PowerShell script syntax check failed";
}
#endif

// ============================================================================
// PLATFORM AGNOSTIC LOGIC (Simulated)
// ============================================================================

// We can't easily test actual installation without Admin rights and messing up
// the dev machine. We will rely on manual verification or a "DryRun" mode in
// the script if implemented.

// ============================================================================
// LINUX TESTS (File existence only on Windows)
// ============================================================================

TEST_F(InstallationTest, LinuxScriptExists) {
  fs::path script_path = fs::path(scripts_dir) / "linux" / "install_client.sh";
  // We expect this to fail initially (TDD)
  EXPECT_TRUE(fs::exists(script_path)) << "Linux install script missing";
}

// ============================================================================
// MACOS TESTS (File existence only on Windows)
// ============================================================================

TEST_F(InstallationTest, MacOsScriptExists) {
  fs::path script_path = fs::path(scripts_dir) / "macos" / "install_client.sh";
  // We expect this to fail initially (TDD)
  EXPECT_TRUE(fs::exists(script_path)) << "MacOS install script missing";
}
