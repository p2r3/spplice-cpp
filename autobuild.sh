#!/bin/bash

target_windows=false
target_both=false

for arg in "$@"; do
  if [ "$arg" == "--win" ]; then
    target_windows=true
  elif [ "$arg" == "--both" ]; then
    target_both=true
  fi
done

# Pull, configure, and statically build Qt5.
# Doing this, of course, assumes you agree to comply with the Qt open source licence.
if [ ! -d "./qt5build" ]; then
  if [ "$target_both" == true ]; then
    ./qt5setup.sh --both
  elif [ "$target_windows" == true ]; then
    ./qt5setup.sh --win
  else
    ./qt5setup.sh
  fi
fi

# Compile UI files into headers.
cd ui
../qt5build/linux/bin/uic -o mainwindow.h MainWindow.ui
../qt5build/linux/bin/uic -o packageitem.h PackageItem.ui
../qt5build/linux/bin/uic -o errordialog.h ErrorDialog.ui
../qt5build/linux/bin/uic -o packageinfo.h PackageInfo.ui
cd ..

# Clear any build cache - building it is fast enough to not really need a cache.
rm -rf build
mkdir build
cd build

if [ "$target_both" == true ]; then

  # Build for both platforms back to back.
  sed -i "s|^#define TARGET_WINDOWS|// #define TARGET_WINDOWS|" ../globals.h
  cmake ..
  make -j$(nproc)
  mv ./SppliceCPP ../build_SppliceCPP
  rm -rf ./*

  sed -i "s|^// #define TARGET_WINDOWS|#define TARGET_WINDOWS|" ../globals.h
  cmake -DCMAKE_TOOLCHAIN_FILE="../windows.cmake" ..
  make -j$(nproc)
  mv ./SppliceCPP.exe ../build_SppliceCPP.exe
  rm -rf ./*

  mv ../build_SppliceCPP ./SppliceCPP
  mv ../build_SppliceCPP.exe ./SppliceCPP.exe

else

  # Build for the specified platform using CMake.
  if [ "$target_windows" == true ]; then
    cmake -DCMAKE_TOOLCHAIN_FILE="../windows.cmake" ..
  else
    cmake ..
  fi
  make -j$(nproc)

fi

cd ..
