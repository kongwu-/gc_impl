cmake_minimum_required(VERSION 3.15)
project(gc_impl C)

set(CMAKE_C_STANDARD 99)

include_directories(mark-sweep)
include_directories(reference-counting)
include_directories(copying)
include_directories(mark-compact)
include_directories(generational)

add_executable(mark-sweep
        mark-sweep/gc.c
        mark-sweep/gc.h
        mark-sweep/test.c)

add_executable(reference-counting
        reference-counting/gc.c
        reference-counting/gc.h
        reference-counting/test.c)

add_executable(copying
        copying/gc.c
        copying/gc.h
        copying/test.c)

add_executable(mark-compact
        mark-compact/gc.c
        mark-compact/gc.h
        mark-compact/test.c)

add_executable(generational
        generational/gc.c
        generational/gc.h
        generational/test.c)