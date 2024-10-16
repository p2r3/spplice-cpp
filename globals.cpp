#include <filesystem>
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

// Points to the external repository file
const std::filesystem::path REPO_PATH = APP_DIR / "repositories.txt";

// Holds the current package installation state
int SPPLICE_INSTALL_STATE = 0; // 0 - idle; 1 - installing; 2 - installed
// TCP communication port between Portal 2 and Spplice
const int SPPLICE_NETCON_PORT = 22333;
