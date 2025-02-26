#ifndef GLOBALS_H
#define GLOBALS_H

#include <filesystem>
#include <fstream>

// Comment this line to target Linux, uncomment to target Windows
#define TARGET_WINDOWS

extern std::filesystem::path CACHE_DIR;
extern bool CACHE_ENABLE;
extern const std::filesystem::path APP_DIR;
extern std::filesystem::path GAME_DIR;
extern std::ofstream LOGFILE;
extern const std::filesystem::path REPO_PATH;
extern int SPPLICE_INSTALL_STATE;
extern int SPPLICE_NETCON_PORT;
extern const std::string SPPLICE_VERSION_TAG;

extern std::string SPPLICE_STEAMAPP_NAMES[];
extern std::string SPPLICE_STEAMAPP_DIRS[];
extern int SPPLICE_STEAMAPP_APPIDS[];
extern const int SPPLICE_STEAMAPP_COUNT;
extern int SPPLICE_STEAMAPP_INDEX;

#endif
