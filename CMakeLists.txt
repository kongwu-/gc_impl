cmake_minimum_required(VERSION 3.15)
project(gc_impl C)

set(CMAKE_C_STANDARD 11)

include_directories(mark-sweep)

add_executable(gc_impl
        mark-sweep/gc.c
        mark-sweep/gc.h
        mark-sweep/test.c)
