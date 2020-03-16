cmake_minimum_required(VERSION 3.15)
project(gc_impl C)

set(CMAKE_C_STANDARD 99)

#include_directories(generational)
#include_directories(mark-sweep)
include_directories(reference-couting)

add_executable(gc_impl
        reference-couting/gc.c
        reference-couting/gc.h
        reference-couting/test.c)
