cmake_minimum_required(VERSION 3.27)
project(SQL C)

set(CMAKE_C_STANDARD 11)

set(CMAKE_C_COMPILER "D:/Codespace/SDKs/C-Cpp/mingw64/bin/gcc.exe")

add_executable(main
        src/main.c
        src/smi.h
        src/smi.c
)