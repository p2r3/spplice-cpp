#!/usr/bin/env bash
set -e

build_root=${BUILD_ROOT:-"$PWD/.."}
mode=clone_build
target_windows=false
target_linux=false

for arg in "$@"; do
  if [ "$arg" == "--win" ] || [ "$arg" == "--windows" ]; then
  echo "Building Qt5 for Windows..."
    target_windows=true
  elif [ "$arg" == "--both" ]; then
    echo "Building Qt5 for Linux and Windows..."
    target_windows=true
    target_linux=true
  elif [ "$arg" == "--linux" ]; then
    echo "Building Qt5 for Linux..."
    target_linux=true
  fi

  if [ "$arg" == "--init-only" ]; then
    echo "Only initializing Qt5 build environment..."
    mode=clone
  elif [ "$arg" == "--build-only" ]; then
    echo "Only building Qt5..."
    mode=build
  fi
done

if [ "$#" -eq 0 ]; then
  echo "No arguments provided. Assuming linux build..."
  target_linux=true
fi

# Return to the project root directory.
echo "Using build root: $build_root"
cd $build_root

if [ "$target_windows" == true ]; then
  rm -rf $build_root/qt5build/win32
elif [ "$target_linux" == true ]; then
  rm -rf $build_root/qt5build/linux
fi

# Ensure a clean start.
if [[ "$mode" == *"clone"* ]]; then

  rm -rf $build_root/qt5

  # Clone the Qt5 repository from their provided git URL.
  git clone git://code.qt.io/qt/qt5.git
  cd $build_root/qt5

  # Checkout to the last available Qt5 LTS version (5.15).
  # This project is built on Qt5, and Qt6 isn't backwards compatible.
  git checkout 5.15
  ./init-repository --module-subset=qtbase

else 
  cd $build_root/qt5
fi


mkdir -p $build_root/qt5/build
if [[ "$mode" == *"build"* ]]; then

  cd $build_root/qt5/build
  set +e
  make clean || true
  set -e

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
      -prefix "$build_root/qt5build/linux" \
      -silent
  }

  function configure_win32 {
    export HOST_COMPILER=g++
    export CROSS_COMPILE=x86_64-w64-mingw32-
    ../configure -release -opensource -confirm-license \
      -qt-libjpeg -qt-libpng -no-gif \
      -qt-pcre \
      -no-icu \
      -no-sqlite \
      -optimize-size \
      -widgets \
      -skip webengine -nomake tools -nomake tests -nomake examples \
      -prefix "$build_root/qt5build/win32" \
      -platform linux-g++ \
      -xplatform win32-g++ \
      -device-option CROSS_COMPILE=$CROSS_COMPILE \
      -device-option HOST_COMPILER=$HOST_COMPILER \
      -opengl desktop \
      -silent
  }

  if [ "$target_linux" == true ]; then
    configure_linux
  elif [ "$target_windows" == true ]; then
    configure_win32
  fi

  make -j$(nproc)
  make install

  # Run an admittedly dirty s/e/d to reroute the find_library CMake function.
  # This lets us cherry-pick what to do with some Qt dependencies at the last minute.
  if [ "$target_linux" == true ]; then
    cd $build_root/qt5build/linux
    find ./ -type f -exec sed -i "s/find_library/find_library_override/g" {} \;
  fi

fi

cd $build_root
