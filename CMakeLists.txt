cmake_minimum_required(VERSION 4.2)
project(wvm C)

set(CMAKE_C_STANDARD 99)

add_executable(wvm
        main.c
        src/bytecode/opcode.h
        src/wvm.h
)
