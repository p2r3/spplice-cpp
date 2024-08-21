#ifndef TOOLS_INSTALL_H
#define TOOLS_INSTALL_H

#include <filesystem>
#include <functional>
#include "../globals.h" // Project globals
#include "package.h" // ToolsPackage

class ToolsInstall {
  public:
    static bool extractLocalFile (const std::filesystem::path path, const std::filesystem::path dest);
#ifndef TARGET_WINDOWS
    static std::string getProcessPath (const std::string &processName);
    static std::pair<bool, std::string> installPackageFile (const std::filesystem::path packageFile);
    static void Uninstall (const std::string &gamePath);
#else
    static std::wstring getProcessPath (const std::string &processName);
    static std::pair<bool, std::wstring> installPackageFile (const std::filesystem::path packageFile);
    static void Uninstall (const std::wstring &gamePath);
#endif // ifndef TARGET_WINDOWS
};

#endif
