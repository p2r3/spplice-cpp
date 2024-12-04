#ifndef TOOLS_INSTALL_H
#define TOOLS_INSTALL_H

#include <filesystem>
#include <functional>
#include "../globals.h" // Project globals
#include "package.h" // ToolsPackage

class ToolsInstall {
  public:
    static bool extractLocalFile (const std::filesystem::path path, const std::filesystem::path dest);
    static std::string installPackageFile (const std::filesystem::path packageFile, const std::vector<std::string> args);
    static bool killPortal2 ();
    static void uninstall ();
#ifndef TARGET_WINDOWS
    static std::string getProcessPath (const std::string &processName);
#else
    static std::wstring getProcessPath (const std::string &processName);
#endif
};

#endif
