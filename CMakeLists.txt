cmake_minimum_required(VERSION 3.10)
project(fishnode C CXX)
set(CMAKE_CXX_STANDARD 23)
add_executable(fishnode src/fishnode.c
        src/fishnode.h
        src/fish.h)