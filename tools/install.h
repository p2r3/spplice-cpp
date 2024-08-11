#ifndef TOOLS_INSTALL_H
#define TOOLS_INSTALL_H

#include <filesystem>
#include <functional>
#include "package.h" // ToolsPackage

class ToolsInstall {
  public:
    static bool extractLocalFile (const std::filesystem::path path, const std::filesystem::path dest);
    static std::string getProcessPath (const std::string &processName);
    static std::pair<bool, std::string> installTempFile ();
    static void Uninstall (const std::string &gamePath);
};

#endif
