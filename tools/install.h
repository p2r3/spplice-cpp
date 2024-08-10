#ifndef TOOLS_INSTALL_H
#define TOOLS_INSTALL_H

#include <filesystem>
#include <functional>
#include "package.h" // ToolsPackage

class ToolsInstall {
  public:
    static bool extractLocalFile (const std::filesystem::path path, const std::filesystem::path dest);
    static void installTempFile (std::function<void(const std::string)> failCallback, std::function<void()> successCallback);
    static void installFromData (ToolsPackage::PackageData *package);
};

#endif
