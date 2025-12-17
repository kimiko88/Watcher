#ifndef CMS_PLATFORM_WINDOWS_SERVICE_H
#define CMS_PLATFORM_WINDOWS_SERVICE_H

#ifdef _WIN32

#include <functional>
#include <string>
#include <windows.h>


namespace cms {
namespace platform {

class WindowsService {
public:
  using ServiceMainCallback = std::function<void()>;
  using ServiceStopCallback = std::function<void()>;

  static void Run(const std::string &serviceName,
                  ServiceMainCallback mainCallback,
                  ServiceStopCallback stopCallback);

private:
  static std::string serviceName_;
  static SERVICE_STATUS_HANDLE serviceStatusHandle_;
  static SERVICE_STATUS serviceStatus_;
  static ServiceMainCallback mainCallback_;
  static ServiceStopCallback stopCallback_;

  static void WINAPI ServiceMain(DWORD dwArgc, LPTSTR *lpszArgv);
  static void WINAPI ServiceCtrlHandler(DWORD dwControl);
  static void ReportStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode,
                           DWORD dwWaitHint);
};

} // namespace platform
} // namespace cms

#endif // _WIN32
#endif // CMS_PLATFORM_WINDOWS_SERVICE_H
