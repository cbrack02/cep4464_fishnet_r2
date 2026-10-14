cmake_minimum_required(VERSION 3.10)
project(fishnode C CXX)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
add_library(fish STATIC IMPORTED)
set_target_properties(fish PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/src/libfish-Linux-x86_64.a" INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/src")
add_executable(fishnode src/fishnode.c
        src/fishnode.h
        src/fish.h
        src/smartalloc.h
        src/smartalloc.c)
target_link_libraries(fishnode PUBLIC fish)
add_compile_definitions(L2_IMPL)
target_compile_options(fishnode PRIVATE -Wall -Wextra -Wpedantic)