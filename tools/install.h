#ifndef TOOLS_INSTALL_H
#define TOOLS_INSTALL_H

#include <filesystem>
#include <functional>
#include "package.h" // ToolsPackage

class ToolsInstall {
  public:
    static bool extractLocalFile (const std::filesystem::path path, const std::filesystem::path dest);
    static std::wstring getProcessPath (const std::string &processName);
    static std::pair<bool, std::wstring> installTempFile ();
    static void Uninstall (const std::wstring &gamePath);
};

#endif
