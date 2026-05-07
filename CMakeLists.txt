cmake_minimum_required(VERSION 3.10)
project(wvm C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS_RELEASE "-O3 -march=native -flto")
set(CMAKE_C_FLAGS_DEBUG "-O3 -march=native")

add_compile_options(-Wall -Wextra -Wpedantic)

add_executable(wvm
        main.c
        src/bytecode/opcode.h
        src/cpu/cpu.c
        src/cpu/cpu.h
        src/memory/memory.h
        src/memory/ram/ram.c
        src/memory/ram/ram.h
        src/memory/register/register.h
        src/asm/assembler.h
        src/asm/assembler.c
        src/wvm.h
)
