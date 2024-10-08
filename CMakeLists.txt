# The dependencies here are modelled after a guided VirtualBox install of Debian, with only Steam installed.
# This saves us from static library linking hell, and allows us to make some assumptions.
# The main assumption we're making is that if standard-ish Debian has it, most distros have it.

cmake_minimum_required(VERSION 3.5)

project(SppliceCPP)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(BUILD_SHARED_LIBS OFF)

# Use link time optimization to reduce executable size.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto")

# This is used along with an s/e/d to force linking of specific dependencies statically throughout Qt.
# Currently, that's just xinerama. Other libraries bloat the output binary by a LOT.
set(FIND_LIBRARY_OVERRIDE_LIST "xcb-xinerama")
set(ORIGINAL_SUFFIXES "${CMAKE_FIND_LIBRARY_SUFFIXES}")

function(find_library_override out_var)

  # Check if the given library is in the override list.
  set(LIB_SUBSTR_FOUND FALSE)
  foreach(ELEMENT IN LISTS FIND_LIBRARY_OVERRIDE_LIST)
    string(FIND "${out_var}" "${ELEMENT}" RESULT)
    if (RESULT GREATER -1)
      set(LIB_SUBSTR_FOUND TRUE)
      break()
    endif()
  endforeach()

  # If found, switch suffixes to those used for static libs.
  if (LIB_SUBSTR_FOUND)
    message(STATUS "Forcing static linking of ${out_var}")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".lib")
  endif()

  # Perform the actual library search.
  find_library(${out_var} NAMES ${ARGN})

  # Restore original suffixes.
  set(CMAKE_FIND_LIBRARY_SUFFIXES "${ORIGINAL_SUFFIXES}")

endfunction()

# Set the exact Qt path (what "qt5setup.sh" builds) for the respective platform
if (SPPLICE_TARGET_WINDOWS)
  set(Qt5Widgets_DIR "${CMAKE_CURRENT_SOURCE_DIR}/qt5build/win32/lib/cmake/Qt5Widgets")
else()
  set(Qt5Widgets_DIR "${CMAKE_CURRENT_SOURCE_DIR}/qt5build/linux/lib/cmake/Qt5Widgets")
endif()

# Find Qt5Widgets.
find_package(Qt5 REQUIRED COMPONENTS Widgets)

# Add resources file
qt_add_resources(RESOURCES resources.qrc)

# Add compilable objects to the executable.
add_executable(SppliceCPP
  main.cpp
  globals.cpp
  tools/curl.cpp
  tools/qt.cpp
  tools/install.cpp
  tools/package.cpp
  tools/repo.cpp
  tools/js.cpp
  tools/netcon.cpp
  deps/shared/duktape/duktape.c
  ${RESOURCES}
)

# Link Qt5Widgets
target_link_libraries(SppliceCPP Qt5::Widgets)

# Finally, link our project's individual dependencies.
if (SPPLICE_TARGET_WINDOWS) # If building for Windows:

  target_link_libraries(SppliceCPP
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/win32/lib/archive.dll"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/win32/lib/liblzma.dll"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/win32/lib/libcurl-x64.dll"
    "libwininet.a"
    "libws2_32.a"
  )

else() # If building for Linux:

  # Locate the statically linkable files for these, as they might be missing on some systems.
  find_library(libacl_STATIC libacl.a)
  find_library(libarchive_STATIC libarchive.a)
  find_library(liblzma_STATIC liblzma.a)

  # Then, link the libraries to our binary.
  target_link_libraries(SppliceCPP
    # Dynamically linked libraries
    nghttp2
    idn2
    psl
    ssl
    crypto
    zstd
    z
    # Statically linked libraries
    ${libacl_STATIC}
    ${libarchive_STATIC}
    ${liblzma_STATIC}
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/linux/lib/libcurl.a"
  )

endif()
