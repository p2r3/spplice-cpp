#ifndef GLOBALS_H
#define GLOBALS_H

#include <filesystem>

// Comment this line to target Linux, uncomment to target Windows
// #define TARGET_WINDOWS

extern const std::filesystem::path TEMP_DIR;
extern int SPPLICE_INSTALL_STATE;
extern int SPPLICE_NETCON_PORT;

#endif
