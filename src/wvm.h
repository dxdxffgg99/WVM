#ifndef WVM_H
#define WVM_H

#include "memory/memory.h"
#include "cpu/cpu.h"
#include "bytecode/opcode.h"
#include <string.h>

static void load_program(CPU* cpu, const uint8_t* code, size_t size) {
    memcpy(cpu->ram.data, code, size);
    cpu->pc = 0;
}

#endif
