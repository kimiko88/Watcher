#include "cms/platform/WindowsService.h"

#ifdef _WIN32

#include <iostream>

namespace cms {
namespace platform {

std::string WindowsService::serviceName_;
SERVICE_STATUS_HANDLE WindowsService::serviceStatusHandle_;
SERVICE_STATUS WindowsService::serviceStatus_;
WindowsService::ServiceMainCallback WindowsService::mainCallback_;
WindowsService::ServiceStopCallback WindowsService::stopCallback_;

void WindowsService::Run(const std::string &serviceName,
                         ServiceMainCallback mainCallback,
                         ServiceStopCallback stopCallback) {
  serviceName_ = serviceName;
  mainCallback_ = mainCallback;
  stopCallback_ = stopCallback;

  SERVICE_TABLE_ENTRYA dispatchTable[] = {
      {const_cast<LPSTR>(serviceName_.c_str()),
       (LPSERVICE_MAIN_FUNCTIONA)ServiceMain},
      {NULL, NULL}};

  if (!StartServiceCtrlDispatcherA(dispatchTable)) {
    std::cerr << "StartServiceCtrlDispatcherA failed (" << GetLastError() << ")"
              << std::endl;
  }
}

void WINAPI WindowsService::ServiceMain(DWORD dwArgc, LPTSTR *lpszArgv) {
  serviceStatusHandle_ =
      RegisterServiceCtrlHandlerA(serviceName_.c_str(), ServiceCtrlHandler);

  if (!serviceStatusHandle_) {
    return;
  }

  // Change Working Directory to Executable Directory
  // This is crucial because Windows Services start in System32 by default
  char path[MAX_PATH];
  if (GetModuleFileNameA(NULL, path, MAX_PATH) > 0) {
    std::string exePath(path);
    std::string exeDir = exePath.substr(0, exePath.find_last_of("\\/"));
    SetCurrentDirectoryA(exeDir.c_str());
    std::cout << "Service CWD set to: " << exeDir << std::endl;
  }

  serviceStatus_.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
  serviceStatus_.dwServiceSpecificExitCode = 0;

  ReportStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

  // Allow user initialization if needed here, effectively declaring we are
  // running
  ReportStatus(SERVICE_RUNNING, NO_ERROR, 0);

  if (mainCallback_) {
    mainCallback_();
  }

  // When mainCallback returns, the service is stopping
  ReportStatus(SERVICE_STOPPED, NO_ERROR, 0);
}

void WINAPI WindowsService::ServiceCtrlHandler(DWORD dwControl) {
  switch (dwControl) {
  case SERVICE_CONTROL_STOP:
    ReportStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
    if (stopCallback_) {
      stopCallback_();
    }
    ReportStatus(SERVICE_STOPPED, NO_ERROR, 0);
    return;
  case SERVICE_CONTROL_INTERROGATE:
    break;
  default:
    break;
  }
}

void WindowsService::ReportStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode,
                                  DWORD dwWaitHint) {
  static DWORD dwCheckPoint = 1;

  serviceStatus_.dwCurrentState = dwCurrentState;
  serviceStatus_.dwWin32ExitCode = dwWin32ExitCode;
  serviceStatus_.dwWaitHint = dwWaitHint;

  if (dwCurrentState == SERVICE_START_PENDING) {
    serviceStatus_.dwControlsAccepted = 0;
  } else {
    serviceStatus_.dwControlsAccepted = SERVICE_ACCEPT_STOP;
  }

  if ((dwCurrentState == SERVICE_RUNNING) ||
      (dwCurrentState == SERVICE_STOPPED)) {
    serviceStatus_.dwCheckPoint = 0;
  } else {
    serviceStatus_.dwCheckPoint = dwCheckPoint++;
  }

  SetServiceStatus(serviceStatusHandle_, &serviceStatus_);
}

} // namespace platform
} // namespace cms

#endif // _WIN32
