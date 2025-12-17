#include "cms/ApplicationManager.h"
#include <iostream>

namespace cms {

std::string ApplicationManager::resolvePathFromPid(uint32_t process_id) const {
  return "";
}

bool ApplicationManager::terminateApplication(uint32_t process_id) {
  return false;
}

bool ApplicationManager::blockRunningApplication(uint32_t process_id) {
  return false;
}

bool ApplicationManager::exportRules(const std::string &filepath) {
  return false;
}

bool ApplicationManager::importRules(const std::string &filepath) {
  return false;
}

} // namespace cms
