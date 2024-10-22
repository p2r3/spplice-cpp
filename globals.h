#ifndef GLOBALS_H
#define GLOBALS_H

#include <filesystem>

// Comment this line to target Linux, uncomment to target Windows
// #define TARGET_WINDOWS

extern std::filesystem::path CACHE_DIR;
extern bool CACHE_ENABLE;
extern const std::filesystem::path APP_DIR;
extern const std::filesystem::path REPO_PATH;
extern int SPPLICE_INSTALL_STATE;
extern int SPPLICE_NETCON_PORT;

#endif
