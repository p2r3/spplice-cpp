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
../qt5build/linux/bin/uic -o repositories.h Repositories.ui
cd ..

# Build application dependencies if not present
if [ ! -d "./deps" ]; then
  ./deps.sh
fi

rm -rf build
mkdir build

# Create a directory for the distributable files.
mkdir dist

cd build

if [ "$target_both" == true ]; then

  rm -rf ../dist/linux; mkdir ../dist/linux
  rm -rf ../dist/win32; mkdir ../dist/win32

  # Build for both platforms back to back.
  sed -i "s|^#define TARGET_WINDOWS|// #define TARGET_WINDOWS|" ../globals.h
  cmake ..
  make -j$(nproc)
  mv ./SppliceCPP ../dist/linux/SppliceCPP
  rm -rf ./*

  sed -i "s|^// #define TARGET_WINDOWS|#define TARGET_WINDOWS|" ../globals.h
  cmake -DCMAKE_TOOLCHAIN_FILE="../windows.cmake" ..
  make -j$(nproc)
  mv ./SppliceCPP.exe ../dist/win32/SppliceCPP.exe

else

  # Build for the specified platform using CMake.
  if [ "$target_windows" == true ]; then
    rm -rf ../dist/win32; mkdir ../dist/win32
    cmake -DCMAKE_TOOLCHAIN_FILE="../windows.cmake" ..
  else
    rm -rf ../dist/linux; mkdir ../dist/linux
    cmake ..
  fi
  make -j$(nproc)

  # Move the output binary to distributables directory.
  if [ "$target_windows" == true ]; then
    mv ./SppliceCPP.exe ../dist/win32/SppliceCPP.exe
  else
    mv ./SppliceCPP ../dist/linux/SppliceCPP
  fi

fi

# Clear any residual build cache.
cd ..
rm -rf build

cd dist

# Prepare the Windows binary for distribution.
if [ "$target_windows" == true ] || [ "$target_both" == true ]; then
  # Copy project dependencies
  cp ../deps/win32/lib/libcurl-4.dll                               ./win32
  cp ../deps/win32/lib/archive.dll                                 ./win32
  cp ../deps/win32/lib/liblzma.dll                                 ./win32
  cp ../deps/win32/lib/libcrypto-1_1-x64.dll                       ./win32
  # Copy Qt5 dependencies
  cp ../qt5build/win32/bin/Qt5Core.dll                             ./win32
  cp ../qt5build/win32/bin/Qt5Gui.dll                              ./win32
  cp ../qt5build/win32/bin/Qt5Widgets.dll                          ./win32
  # Copy Qt5 plugins
  mkdir ./win32/platforms
  cp -r ../qt5build/win32/plugins/platforms/qwindows.dll           ./win32/platforms
  cp -r ../qt5build/win32/plugins/imageformats                     ./win32
  # Copy C++ dependencies
  cp /usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll               ./win32
  cp /usr/lib/gcc/x86_64-w64-mingw32/12-posix/libgcc_s_seh-1.dll   ./win32
  cp /usr/lib/gcc/x86_64-w64-mingw32/12-posix/libstdc++-6.dll      ./win32
fi

if [ "$target_windows" == false ] || [ "$target_both" == true ]; then
  # Pack with UPX
  ../deps/shared/upx/upx --best --lzma ./linux/SppliceCPP
fi
