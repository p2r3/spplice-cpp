#ifndef TOOLS_INSTALL_H
#define TOOLS_INSTALL_H

#include <filesystem>
#include <functional>

class ToolsInstall {
  public:
    static bool extractLocalFile (const std::filesystem::path path, const std::filesystem::path dest);
    static void installRemoteFile (const std::string &fileURL, std::function<void(const std::string)> failCallback, std::function<void()> successCallback);
};

#endif
