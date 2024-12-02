#include <filesystem>
#include <fstream>
#include "globals.h"

// Points to the system-specific designated cache directory
#ifndef TARGET_WINDOWS
std::filesystem::path CACHE_DIR = (std::filesystem::path(std::getenv("HOME")) / ".cache") / "spplice-cpp";
#else
std::filesystem::path CACHE_DIR = std::filesystem::temp_directory_path() / "spplice-cpp";
#endif
bool CACHE_ENABLE = true;

// Points to the system-specific designated application directory
#ifndef TARGET_WINDOWS
const std::filesystem::path APP_DIR = (std::filesystem::path(std::getenv("HOME")) / ".config") / "spplice-cpp";
#else
const std::filesystem::path APP_DIR = std::filesystem::path(std::getenv("APPDATA")) / "spplice-cpp";
#endif

// Points to last known Portal 2 game files directory
std::filesystem::path GAME_DIR;
// Points to info/error log file (set during runtime)
std::ofstream LOGFILE;

// Points to the external repository file
const std::filesystem::path REPO_PATH = APP_DIR / "repositories.txt";

// Holds the current package installation state
int SPPLICE_INSTALL_STATE = 0; // 0 - idle; 1 - installing; 2 - installed
// TCP communication port between Portal 2 and Spplice (set during runtime)
int SPPLICE_NETCON_PORT = -1;
