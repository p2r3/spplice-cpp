#ifndef TOOLS_INSTALL_H
#define TOOLS_INSTALL_H

#include <filesystem>

class ToolsInstall {
  public:
    static bool extractLocalFile (const std::filesystem::path path, const std::filesystem::path dest);
    static void installRemoteFile (const std::string &fileURL);
};

#endif
