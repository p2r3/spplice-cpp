#!/usr/bin/env bash
set -e

build_root=${BUILD_ROOT:-"$PWD"}

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
done

if [ "$#" -eq 0 ]; then
  echo "No arguments provided. Assuming Linux build..."
  target_linux=true
fi


# Pull, configure, and statically build Qt5.
# Doing this, of course, assumes you agree to comply with the Qt open source licence.
if [ ! -d "$build_root/qt5build" ]; then
  cd $build_root/scripts

  if [ "$target_both" == true ]; then
    ./qt5setup.sh --both
  elif [ "$target_windows" == true ]; then
    ./qt5setup.sh --win
  else
    ./qt5setup.sh
  fi

  cd $build_root
fi


echo "Using build root: $build_root"
cd $build_root

# Compile UI files into headers.
echo "Compiling UI files..."
cd $build_root/ui
ui_path="win32";
if [ "$target_linux" == true ]; then
  ui_path="linux";
fi

$build_root/qt5build/$ui_path/bin/uic -o mainwindow.h MainWindow.ui
$build_root/qt5build/$ui_path/bin/uic -o packageitem.h PackageItem.ui
$build_root/qt5build/$ui_path/bin/uic -o errordialog.h ErrorDialog.ui
$build_root/qt5build/$ui_path/bin/uic -o packageinfo.h PackageInfo.ui
$build_root/qt5build/$ui_path/bin/uic -o repositories.h Repositories.ui
$build_root/qt5build/$ui_path/bin/uic -o settings.h Settings.ui

cd $build_root

# Build application dependencies if not present
if [ ! -d "./deps" ]; then
  echo "Building dependencies..."
  cd scripts
  ./deps.sh
  cd $build_root
fi

rm -rf build
mkdir build

# Create a directory for the distributable files.
mkdir -p dist

cd build

# Build for the specified platform using CMake.
if [ "$target_windows" == true ]; then
  rm -rf $build_root/dist/win32; mkdir $build_root/dist/win32
  sed -i "s|^// #define TARGET_WINDOWS|#define TARGET_WINDOWS|" $build_root/globals.h
  cmake -DCMAKE_TOOLCHAIN_FILE="$build_root/scripts/windows.cmake" $build_root/scripts

  make -j$(nproc)

  mv ./SppliceCPP.exe $build_root/dist/win32/SppliceCPP.exe

elif [ "$target_linux" == true ]; then
  rm -rf $build_root/dist/linux; mkdir $build_root/dist/linux
  sed -i "s|^#define TARGET_WINDOWS|// #define TARGET_WINDOWS|" $build_root/globals.h
  cmake $build_root/scripts

  make -j$(nproc)

  mv ./SppliceCPP $build_root/dist/linux/SppliceCPP
fi

# Clear any residual build cache.
cd $build_root
rm -rf build

cd dist

# Prepare the Windows binary for distribution.
if [ "$target_windows" == true ]; then
  # Copy project dependencies
  cp $build_root/deps/win32/lib/libcurl-4.dll                      ./win32
  cp $build_root/deps/win32/lib/archive.dll                        ./win32
  cp $build_root/deps/win32/lib/liblzma.dll                        ./win32
  cp $build_root/deps/win32/lib/libcrypto-1_1-x64.dll              ./win32
  # Copy Qt5 dependencies
  cp $build_root/qt5build/win32/bin/Qt5Core.dll                    ./win32
  cp $build_root/qt5build/win32/bin/Qt5Gui.dll                     ./win32
  cp $build_root/qt5build/win32/bin/Qt5Widgets.dll                 ./win32
  # Copy Qt5 plugins
  mkdir ./win32/platforms
  cp -r $build_root/qt5build/win32/plugins/platforms/qwindows.dll  ./win32/platforms
  cp -r $build_root/qt5build/win32/plugins/imageformats            ./win32
  # Copy C++ dependencies
  cp /usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll               ./win32
  cp /usr/lib/gcc/x86_64-w64-mingw32/12-posix/libgcc_s_seh-1.dll   ./win32
  cp /usr/lib/gcc/x86_64-w64-mingw32/12-posix/libstdc++-6.dll      ./win32
  # Strip debug symbols
  strip ./win32/SppliceCPP.exe
  strip ./win32/*.dll
  strip ./win32/platforms/*.dll
  strip ./win32/imageformats/*.dll
fi

if [ "$target_linux" == true ]; then
  # Strip debug symbols
  strip ./linux/SppliceCPP
  # Pack with UPX
  $build_root/deps/shared/upx/upx --best --lzma ./linux/SppliceCPP
fi
