#include <filesystem>
#include "globals.h"

// Points to the system-specific designated temporary file path
const std::filesystem::path TEMP_DIR = std::filesystem::temp_directory_path() / "cpplice";
// Holds the current package installation state
int SPPLICE_INSTALL_STATE = 0; // 0 - idle; 1 - installing; 2 - installed
