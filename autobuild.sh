#!/bin/bash

target_windows=false

for arg in "$@"; do
  if [ "$arg" == "--win" ]; then
    target_windows=true
    break
  fi
done

# Pull, configure, and statically build Qt5.
# Doing this, of course, assumes you agree to comply with the Qt open source licence.
if [ ! -d "./qt5-static" ]; then
  if [ "$target_windows" == true ]; then
    ./qt5setup.sh --win
  else
    ./qt5setup.sh
  fi
fi

# Compile UI files into headers.
cd ui
../qt5-static/bin/uic -o mainwindow.h MainWindow.ui
../qt5-static/bin/uic -o packageitem.h PackageItem.ui
../qt5-static/bin/uic -o errordialog.h ErrorDialog.ui
../qt5-static/bin/uic -o packageinfo.h PackageInfo.ui
cd ..

# Clear any build cache - building it is fast enough to not really need a cache.
rm -rf build
mkdir build
cd build

# Build the project using CMake.
if [ "$target_windows" == true ]; then
  cmake -DCMAKE_TOOLCHAIN_FILE="../windows.cmake" ..
else
  cmake ..
fi

make
cd ..
