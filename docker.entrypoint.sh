#/usr/bin/env bash

# Exit on errors
set -e

# Clean the dist folder
rm -rf /dist/*

# Create the /work sub-directories
mkdir -p /work/{linux,win32}

# Copy /qt5-win & /qt5-linux to /work
cp -r /qt5-win32/* /work/win32
cp -r /qt5-linux/* /work/linux

# Copy all the content of /src to /work
cp -r /src/* /work/linux
cp -r /src/* /work/win32

# Change to the /work directory
cd /work

# Run autobuild for windows & linux
BUILD_ROOT=/work/win32 ./win32/autobuild.sh --windows &
BUILD_ROOT=/work/linux ./linux/autobuild.sh --linux &

# Wait for autobuilds to finish
wait

# Copy the build artifacts to /dist
cp -r /work/{win32,linux}/dist/* /dist
