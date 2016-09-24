cmake_minimum_required (VERSION 2.6)
project (Mandelbrot)
add_definitions(-std=c++11)
add_executable (Mandelbrot Timer.h TaskQueue.h Main.cpp)
