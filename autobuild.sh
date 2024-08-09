#!/bin/bash

# Pull, configure, and statically build Qt5.
# Doing this, of course, assumes you agree to comply with the Qt open source licence.
if [ ! -d "./qt5-static" ]; then
  ./qt5setup.sh
fi

# Compile UI files into headers.
cd ui
../qt5-static/bin/uic -o mainwindow.h MainWindow.ui
../qt5-static/bin/uic -o packageitem.h PackageItem.ui
../qt5-static/bin/uic -o errordialog.h ErrorDialog.ui
cd ..

# Clear any build cache - building it is fast enough to not really need a cache.
rm -rf build
mkdir build
cd build

# Build the project using CMake
cmake ..
make
cd ..
