CMake is re-running because C:/Users/chimi/Desktop/Programmazione/Watcher/build/src/master/CMakeFiles/generate.stamp is out-of-date.
  the file 'C:/Users/chimi/Desktop/Programmazione/Watcher/src/master/CMakeLists.txt'
  is newer than 'C:/Users/chimi/Desktop/Programmazione/Watcher/build/src/master/CMakeFiles/generate.stamp.depend'
  result='-1'
-- Selecting Windows SDK version 10.0.26100.0 to target Windows 10.0.26200.
-- Building for Windows
-- Build type: Debug
-- Using the multi-header code from C:/Users/chimi/Desktop/Programmazione/Watcher/build/_deps/json-src/include/
-- Searching for FFmpeg libraries...
-- pkg-config not found or FFmpeg not detected, trying manual search...
cmake : CMake Warning at 
CMakeLists.txt:143 (message):
In riga:1 car:1
+ cmake --build build 
--config Release > 
build_log_5.txt 2>&1
+ ~~~~~~~~~~~~~~~~~~~~~~~~~~~
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
~~~
    + CategoryInfo           
   : NotSpecified: (CMake W  
  arning a...:143 (message   
 )::String) [], RemoteExc    
eption
    + FullyQualifiedErrorId  
   : NativeCommandError
 
  FFmpeg NOT found.  Screen 
recording will use stub 
implementation.


CMake Warning at 
CMakeLists.txt:144 (message):
  To enable FFmpeg:


CMake Warning at 
CMakeLists.txt:145 (message):
    - Install FFmpeg from htt
ps://www.gyan.dev/ffmpeg/buil
ds/


CMake Warning at 
CMakeLists.txt:146 (message):
    - Set FFMPEG_ROOT 
environment variable to 
installation path


CMake Warning at 
CMakeLists.txt:147 (message):
    - Or install via vcpkg: 
vcpkg install ffmpeg


-- Configured cms_core library
-- Building for Windows platform
-- Configured cms_platform library with sources: WindowsPlatform.cpp;WindowsService.cpp;ScreenCaptureService.cpp;InputLockManager.cpp;Windows/ActivityMonitorWindows.cpp;Windows/ApplicationManagerWindows.cpp
-- Configured cms_client_service and cms_client_worker executables
-- Configured cms_master_service executable (lightweight, no MasterServer)
-- Qt6 found. Building cms_master GUI.
-- Configured cms_master executable
-- 
-- === Classroom Control (CMS) Build Configuration ===
-- Version: 1.0.0
-- C++ Standard: C++17
-- Build Type: Debug
-- Compiler: MSVC 19.44.35222.0
-- ====================================================
-- 
-- Configuring done (3.2s)
-- Generating done (2.1s)
-- Build files have been written to: C:/Users/chimi/Desktop/Programmazione/Watcher/build
Versione di MSBuild 17.14.23+b0019275e per .NET Framework

  cms_core.vcxproj -> C:\Users\chimi\Desktop\Programmazione\Watcher\build\src\core\Release\cms_core.lib
  cms_platform.vcxproj -> C:\Users\chimi\Desktop\Programmazione\Watcher\build\src\platform\Release\cms_platform.lib
  ServiceLauncher.cpp
C:\Users\chimi\Desktop\Programmazione\Watcher\src\client\ServiceLauncher.cpp(352,16): error C2039: 'ifstrream': non ├¿ un membro di 'std' [C:\Users\chimi\Desktop\Programmazione\Watcher\build\src\client\cms_client_service.vcxproj]
      C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Tools\MSVC\14.44.35207\include\sstream(19,1):
      vedere la dichiarazione di 'std'
  
C:\Users\chimi\Desktop\Programmazione\Watcher\src\client\ServiceLauncher.cpp(352,16): error C2065: 'ifstrream': identificatore non dichiarato [C:\Users\chimi\Desktop\Programmazione\Watcher\build\src\client\cms_client_service.vcxproj]
C:\Users\chimi\Desktop\Programmazione\Watcher\src\client\ServiceLauncher.cpp(352,26): error C2146: errore di sintassi: ';' mancante prima dell'identificatore 'in' [C:\Users\chimi\Desktop\Programmazione\Watcher\build\src\client\cms_client_service.vcxproj]
C:\Users\chimi\Desktop\Programmazione\Watcher\src\client\ServiceLauncher.cpp(352,26): error C3861: 'in': identificatore non trovato [C:\Users\chimi\Desktop\Programmazione\Watcher\build\src\client\cms_client_service.vcxproj]
C:\Users\chimi\Desktop\Programmazione\Watcher\src\client\ServiceLauncher.cpp(356,15): error C2065: 'in': identificatore non dichiarato [C:\Users\chimi\Desktop\Programmazione\Watcher\build\src\client\cms_client_service.vcxproj]
C:\Users\chimi\Desktop\Programmazione\Watcher\src\client\ServiceLauncher.cpp(357,33): error C2065: 'in': identificatore non dichiarato [C:\Users\chimi\Desktop\Programmazione\Watcher\build\src\client\cms_client_service.vcxproj]
  ClientService.cpp
C:\Users\chimi\Desktop\Programmazione\Watcher\src\client\ClientService.cpp(517,33): error C3861: 'base64_decode': identificatore non trovato [C:\Users\chimi\Desktop\Programmazione\Watcher\build\src\client\cms_client_worker.vcxproj]
C:\Users\chimi\Desktop\Programmazione\Watcher\src\client\ClientService.cpp(521,11): error C2653: 'fs' non ├¿ il nome di una classe o di uno spazio dei nomi [C:\Users\chimi\Desktop\Programmazione\Watcher\build\src\client\cms_client_worker.vcxproj]
C:\Users\chimi\Desktop\Programmazione\Watcher\src\client\ClientService.cpp(521,15): error C2065: 'path': identificatore non dichiarato [C:\Users\chimi\Desktop\Programmazione\Watcher\build\src\client\cms_client_worker.vcxproj]
C:\Users\chimi\Desktop\Programmazione\Watcher\src\client\ClientService.cpp(521,20): error C2146: errore di sintassi: ';' mancante prima dell'identificatore 'displayPath' [C:\Users\chimi\Desktop\Programmazione\Watcher\build\src\client\cms_client_worker.vcxproj]
C:\Users\chimi\Desktop\Programmazione\Watcher\src\client\ClientService.cpp(521,20): error C2065: 'displayPath': identificatore non dichiarato [C:\Users\chimi\Desktop\Programmazione\Watcher\build\src\client\cms_client_worker.vcxproj]
C:\Users\chimi\Desktop\Programmazione\Watcher\src\client\ClientService.cpp(521,34): error C2653: 'fs' non ├¿ il nome di una classe o di uno spazio dei nomi [C:\Users\chimi\Desktop\Programmazione\Watcher\build\src\client\cms_client_worker.vcxproj]
C:\Users\chimi\Desktop\Programmazione\Watcher\src\client\ClientService.cpp(521,38): error C3861: 'current_path': identificatore non trovato [C:\Users\chimi\Desktop\Programmazione\Watcher\build\src\client\cms_client_worker.vcxproj]
C:\Users\chimi\Desktop\Programmazione\Watcher\src\client\ClientService.cpp(522,16): error C2653: 'fs' non ├¿ il nome di una classe o di uno spazio dei nomi [C:\Users\chimi\Desktop\Programmazione\Watcher\build\src\client\cms_client_worker.vcxproj]
C:\Users\chimi\Desktop\Programmazione\Watcher\src\client\ClientService.cpp(522,27): error C2065: 'displayPath': identificatore non dichiarato [C:\Users\chimi\Desktop\Programmazione\Watcher\build\src\client\cms_client_worker.vcxproj]
C:\Users\chimi\Desktop\Programmazione\Watcher\src\client\ClientService.cpp(522,20): error C3861: 'exists': identificatore non trovato [C:\Users\chimi\Desktop\Programmazione\Watcher\build\src\client\cms_client_worker.vcxproj]
C:\Users\chimi\Desktop\Programmazione\Watcher\src\client\ClientService.cpp(523,13): error C2653: 'fs' non ├¿ il nome di una classe o di uno spazio dei nomi [C:\Users\chimi\Desktop\Programmazione\Watcher\build\src\client\cms_client_worker.vcxproj]
C:\Users\chimi\Desktop\Programmazione\Watcher\src\client\ClientService.cpp(523,34): error C2065: 'displayPath': identificatore non dichiarato [C:\Users\chimi\Desktop\Programmazione\Watcher\build\src\client\cms_client_worker.vcxproj]
C:\Users\chimi\Desktop\Programmazione\Watcher\src\client\ClientService.cpp(523,17): error C3861: 'create_directory': identificatore non trovato [C:\Users\chimi\Desktop\Programmazione\Watcher\build\src\client\cms_client_worker.vcxproj]
C:\Users\chimi\Desktop\Programmazione\Watcher\src\client\ClientService.cpp(526,11): error C2653: 'fs' non ├¿ il nome di una classe o di uno spazio dei nomi [C:\Users\chimi\Desktop\Programmazione\Watcher\build\src\client\cms_client_worker.vcxproj]
C:\Users\chimi\Desktop\Programmazione\Watcher\src\client\ClientService.cpp(526,15): error C2065: 'path': identificatore non dichiarato [C:\Users\chimi\Desktop\Programmazione\Watcher\build\src\client\cms_client_worker.vcxproj]
C:\Users\chimi\Desktop\Programmazione\Watcher\src\client\ClientService.cpp(526,20): error C2146: errore di sintassi: ';' mancante prima dell'identificatore 'filePath' [C:\Users\chimi\Desktop\Programmazione\Watcher\build\src\client\cms_client_worker.vcxproj]
C:\Users\chimi\Desktop\Programmazione\Watcher\src\client\ClientService.cpp(526,20): error C2065: 'filePath': identificatore non dichiarato [C:\Users\chimi\Desktop\Programmazione\Watcher\build\src\client\cms_client_worker.vcxproj]
C:\Users\chimi\Desktop\Programmazione\Watcher\src\client\ClientService.cpp(526,31): error C2065: 'displayPath': identificatore non dichiarato [C:\Users\chimi\Desktop\Programmazione\Watcher\build\src\client\cms_client_worker.vcxproj]
C:\Users\chimi\Desktop\Programmazione\Watcher\src\client\ClientService.cpp(527,33): error C2065: 'filePath': identificatore non dichiarato [C:\Users\chimi\Desktop\Programmazione\Watcher\build\src\client\cms_client_worker.vcxproj]
C:\Users\chimi\Desktop\Programmazione\Watcher\src\client\ClientService.cpp(532,13): error C2065: 'filePath': identificatore non dichiarato [C:\Users\chimi\Desktop\Programmazione\Watcher\build\src\client\cms_client_worker.vcxproj]
C:\Users\chimi\Desktop\Programmazione\Watcher\src\client\ClientService.cpp(535,60): error C2065: 'filePath': identificatore non dichiarato [C:\Users\chimi\Desktop\Programmazione\Watcher\build\src\client\cms_client_worker.vcxproj]
C:\Users\chimi\Desktop\Programmazione\Watcher\src\client\ClientService.cpp(533,24): error C2660: 'cms::platform::ISystemOperations::showMessageBox': la funzione non accetta 1 argomenti [C:\Users\chimi\Desktop\Programmazione\Watcher\build\src\client\cms_client_worker.vcxproj]
      C:\Users\chimi\Desktop\Programmazione\Watcher\include\cms\Platform.h(183,16):
      vedere la dichiarazione di 'cms::platform::ISystemOperations::showMessageBox'
      C:\Users\chimi\Desktop\Programmazione\Watcher\src\client\ClientService.cpp(533,24):
      durante la ricerca di corrispondenza con l'elenco di argomenti '(const char [14])'
  
C:\Users\chimi\Desktop\Programmazione\Watcher\src\client\ClientService.cpp(537,13): error C2065: 'filePath': identificatore non dichiarato [C:\Users\chimi\Desktop\Programmazione\Watcher\build\src\client\cms_client_worker.vcxproj]
  Automatic MOC and UIC for target cms_master
  Generating __/__/include/cms/ui/moc_LoginDialog.cpp
  moc_LoginDialog.cpp
  cms_master.vcxproj -> C:\Users\chimi\Desktop\Programmazione\Watcher\build\src\master\Release\cms_master.exe
  Automatic MOC for target cms_master_service
  cms_master_service.vcxproj -> C:\Users\chimi\Desktop\Programmazione\Watcher\build\src\master\Release\cms_master_service.exe
  gmock.vcxproj -> C:\Users\chimi\Desktop\Programmazione\Watcher\build\lib\Release\gmock.lib
  gmock_main.vcxproj -> C:\Users\chimi\Desktop\Programmazione\Watcher\build\lib\Release\gmock_main.lib
  gtest.vcxproj -> C:\Users\chimi\Desktop\Programmazione\Watcher\build\lib\Release\gtest.lib
  gtest_main.vcxproj -> C:\Users\chimi\Desktop\Programmazione\Watcher\build\lib\Release\gtest_main.lib
