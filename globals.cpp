#include <filesystem>
#include "globals.h"

// Points to the system-specific designated temporary file path
const std::filesystem::path TEMP_DIR = std::filesystem::temp_directory_path() / "cpplice";
bool BUSY_INSTALLING = false;
