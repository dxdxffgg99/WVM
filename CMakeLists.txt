cmake_minimum_required(VERSION 3.10)
project(wvm C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_executable(wvm
        main.c
        src/bytecode/opcode.h
        src/cpu/cpu.h
        src/memory/memory.h
        src/memory/ram/ram.h
        src/memory/register/register.h
        src/cpu/cpu.c
        src/wvm.h
)
