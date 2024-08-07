cmake_minimum_required(VERSION 3.5)

project(SppliceCPP)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Statically link everything. This makes sense for an all-in-one for-dummies app.
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_FIND_LIBRARY_SUFFIXES .a .so)
# set(CMAKE_EXE_LINKER_FLAGS "-static")

find_package(Qt5Widgets REQUIRED)

add_executable(SppliceCPP
  main.cpp
  globals.cpp
  tools/curl.cpp
  tools/qt.cpp
  tools/install.cpp
  tools/package.cpp
  tools/repo.cpp
)

target_link_libraries(SppliceCPP
  Qt5::Widgets
  curl
  archive
  lzma
)
