#!/bin/bash

target_windows=false

for arg in "$@"; do
  if [ "$arg" == "--win" ]; then
    target_windows=true
    break
  fi
done

if [ "$target_windows" == true ]; then
  echo "Building Qt5 for Windows..."
else
  echo "Building Qt5 for Linux..."
fi

# Ensure a clean start.
rm -rf ./qt5
rm -rf ./qt5-static

# Clone the Qt5 repository from their provided git URL.
git clone git://code.qt.io/qt/qt5.git
cd qt5

# Checkout to the last available Qt5 LTS version (5.15).
# This project is built on Qt5, and Qt6 isn't backwards compatible.
git checkout 5.15
./init-repository --module-subset=qtbase

mkdir build
cd build

# Configure the Qt5 build. There are several notable flags here:
#   - Building with and accepting the open source license, thus also providing the build source and method here;
#   - Statically linkable libraries, as Spplice aims to be a single-binary application;
#   - No precompiled headers;
#   - Qt-provided libjpeg and libpng implementations for better single-binary bundling;
#   - Disabling unneeded GIF support;
#   - Qt-provided PCRE, as it's annoying to link statically after the fact and Qt cannot build without it;
#   - Bundled xinput, which gets rid of a potential dependency;
#   - No ICU support, getting rid of further dependencies under the assumption that we won't need it;
#   - No SQLite, because Spplice doesn't really need a database;
#   - Optimize for file size, as build time and source size are secondary to that;
#   - Link-time code generation, further optimizing the size and efficiency of the application, albeit at some risk;
#   - Include the QWidgets component - that's what we're here for;
#   - Skip building the Qt webengine, tools, tests, and examples;
#   - Install into the local "qt5-static" directory, later used to build the rest of the project.
if [ "$target_windows" == false ]; then

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
    -prefix "$PWD/../../qt5-static"

else

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
    -prefix "$PWD/../../qt5-static" \
    -platform linux-g++ \
    -xplatform win32-g++ \
    -device-option CROSS_COMPILE=$CROSS_COMPILE \
    -device-option HOST_COMPILER=$HOST_COMPILER \
    -opengl desktop \
    -silent

fi

# Run `make` with as many CPU threads as are available.
make -j$(nproc)
make install

# Run an admittedly dirty s/e/d to reroute the find_library CMake function.
# This lets us cherry-pick what to do with some Qt dependencies at the last minute.
if [ "$target_windows" == false ]; then
  cd ../../qt5-static
  find ./ -type f -exec sed -i "s/find_library/find_library_override/g" {} \;
  cd ..
else
  cd ..
fi
