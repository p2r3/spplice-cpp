cmake_minimum_required(VERSION 3.5)

project(SppliceCPP)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Widgets REQUIRED)

add_executable(SppliceCPP
  ui/mainwindow.h
  ui/packageitem.h
  main.cpp
)
target_link_libraries(SppliceCPP Qt5::Widgets curl)
