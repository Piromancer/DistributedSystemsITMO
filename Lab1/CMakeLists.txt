cmake_minimum_required (VERSION 2.6)
project (Lab1)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall -Werror -pedantic")

add_executable(peer
    main.c
    ipc.c
    common.h
    ipc.h
    pa1.h
)