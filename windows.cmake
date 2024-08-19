set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Specify the cross-compiler
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

# Set the Qt directory
set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_LIST_DIR}/qt5-static")

# Specify where to find dependencies
include_directories("${CMAKE_CURRENT_LIST_DIR}/win/include")
link_directories("${CMAKE_CURRENT_LIST_DIR}/win/lib")

set(SPPLICE_TARGET_WINDOWS TRUE)
