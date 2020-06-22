cmake_minimum_required (VERSION 2.6)
project (Lab4)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -L/mnt/d/CLionProjects/DSI/Lab4/lib64 -Wl,-rpath=/mnt/d/CLionProjects/DSI/Lab4/lib64 -std=c99 -Wall -Werror -pedantic")

add_executable(peer4
        banking.h
        common.h
        io.h
        ipc.c
        ipc.h
        main.c
        pa1.h
        pa2345.h)
target_link_libraries(peer4 runtime)