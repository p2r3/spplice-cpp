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

// Holds the current version's GitHub tag for automatic updates
const std::string SPPLICE_VERSION_TAG = "v0.7.1-alpha";

// Contains a list of compatible Steam app names
std::string SPPLICE_STEAMAPP_NAMES[] = {
  "Portal 2",
  "Portal Stories: Mel",
  "Portal Reloaded",
  "Aperture Tag"
};
// Contains a list of mod directory names for supported Steam apps
std::string SPPLICE_STEAMAPP_DIRS[] = {
  "portal2",
  "portal_stories",
  "portalreloaded",
  "aperturetag"
};
// Contains a list of compatible Steam AppIDs
int SPPLICE_STEAMAPP_APPIDS[] = {
  620,
  317400,
  1255980,
  280740
};
// Holds the number of supported Steam apps
const int SPPLICE_STEAMAPP_COUNT = sizeof(SPPLICE_STEAMAPP_APPIDS) / sizeof(*SPPLICE_STEAMAPP_APPIDS);
// Holds the index of the Steam app currently selected in settings
int SPPLICE_STEAMAPP_INDEX = 0;
