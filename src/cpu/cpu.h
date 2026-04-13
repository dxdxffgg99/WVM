#ifndef WVM_CPU_H
#define WVM_CPU_H

#include <stdbool.h>
#include <stdint.h>
#include "../memory/ram/ram.h"
#include "../memory/register/register.h"
#include "../bytecode/opcode.h"

typedef enum {
    CPU_OK = 0,
    CPU_ERR_UNKNOWN_COMMAND,
    CPU_ERR_DIV_ZERO,
    CPU_ERR_OVERFLOW,
} fn;

typedef struct {
    Registers registors;
    RAM ram;

    uint32_t pc;

    bool running;
} CPU;

int cpu_init(CPU* cpu, uint64_t ram_size);

void cpu_free(CPU* cpu);

fn run(CPU* cpu);

#endif
