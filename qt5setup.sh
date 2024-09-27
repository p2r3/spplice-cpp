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

if [ "$target_both" == true ]; then
  echo "Building Qt5 for Linux and Windows..."
elif [ "$target_windows" == true ]; then
  echo "Building Qt5 for Windows..."
else
  echo "Building Qt5 for Linux..."
fi

# Ensure a clean start.
rm -rf ./qt5
rm -rf ./qt5build

# Clone the Qt5 repository from their provided git URL.
git clone git://code.qt.io/qt/qt5.git
cd qt5

# Checkout to the last available Qt5 LTS version (5.15).
# This project is built on Qt5, and Qt6 isn't backwards compatible.
git checkout 5.15
./init-repository --module-subset=qtbase

mkdir build
cd build
make clean

function configure_linux {
  ../configure -release -opensource -confirm-license \
    -static \
    -no-pch \
    -qt-libjpeg -qt-libpng -no-gif \
    -qt-pcre \
    -bundled-xcb-xinput \
    -no-icu \
    -no-sqlite \
    -optimize-size \
    -ltcg \
    -widgets \
    -skip webengine -nomake tools -nomake tests -nomake examples \
    -prefix "$PWD/../../qt5build/linux" \
    -silent
}

function configure_win32 {
  export HOST_COMPILER=g++
  export CROSS_COMPILE=x86_64-w64-mingw32-
  ../configure -release -opensource -confirm-license \
    -static \
    -qt-libjpeg -qt-libpng -no-gif \
    -qt-pcre \
    -no-icu \
    -no-sqlite \
    -optimize-size \
    -widgets \
    -skip webengine -nomake tools -nomake tests -nomake examples \
    -prefix "$PWD/../../qt5build/win32" \
    -platform linux-g++ \
    -xplatform win32-g++ \
    -device-option CROSS_COMPILE=$CROSS_COMPILE \
    -device-option HOST_COMPILER=$HOST_COMPILER \
    -opengl desktop \
    -silent
}

if [ "$target_both" == true ]; then

  configure_linux
  make -j$(nproc)
  make install
  rm -rf ./*

  configure_win32
  make -j$(nproc)
  make install
  rm -rf ./*

else
  if [ "$target_windows" == false ]; then
    configure_linux
  else
    configure_win32
  fi
  make -j$(nproc)
  make install
fi

# Run an admittedly dirty s/e/d to reroute the find_library CMake function.
# This lets us cherry-pick what to do with some Qt dependencies at the last minute.
if [ "$target_windows" == false ] || [ "$target_both" == true ]; then
  cd ../../qt5build/linux
  find ./ -type f -exec sed -i "s/find_library/find_library_override/g" {} \;
  cd ../..
else
  cd ..
fi
