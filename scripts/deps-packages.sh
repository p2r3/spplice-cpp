#!/usr/bin/env bash
set -e

if command -v apt > /dev/null; then

  echo "Checking for necessary -dev packages..."
  sudo apt install -y \
    libarchive-dev \
    libxml2-dev \
    liblzma-dev \
    libacl1-dev \
    libgl1-mesa-dev \
    libxcb*-dev \
    libfontconfig1-dev \
    libxkbcommon-x11-dev \
    libnghttp2-dev \
    libidn2-dev

  echo "Checking for MinGW for cross-compilation..."
  sudo apt install -y \
    mingw-w64 \
    mingw-w64-tools \
    gcc-mingw-w64-x86-64 \
    g++-mingw-w64-x86-64

  echo "Switching to POSIX threads..."
  sudo update-alternatives --set x86_64-w64-mingw32-gcc /usr/bin/x86_64-w64-mingw32-gcc-posix
  sudo update-alternatives --set x86_64-w64-mingw32-g++ /usr/bin/x86_64-w64-mingw32-g++-posix

fi
