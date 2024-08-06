#!/bin/bash

cd ui
uic -o mainwindow.h MainWindow.ui
uic -o packageitem.h PackageItem.ui
cd ..

cd build
cmake ..
make
cd ..

./build/SppliceCPP