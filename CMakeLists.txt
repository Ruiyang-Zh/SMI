cmake_minimum_required(VERSION 3.27)
project(SQL C)

set(CMAKE_C_STANDARD 11)

add_executable(smi smi.c
        smi.h
        main.c)

add_executable(text text.c)