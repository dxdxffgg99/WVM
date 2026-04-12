#ifndef WVM_CPU_H
#define WVM_CPU_H

#include <stdint.h>
#include "../memory/ram/ram.h"
#include "../memory/register/register.h"
#include "../bytecode/opcode.h"

typedef enum {
    CPU_OK = 0,
    CPU_ERR,
    CPU_ERR_DIV_ZERO,
} fn;

typedef enum {
    SYS_EXIT = 0,
    SYS_WRITE = 1,
    SYS_READ = 2,
    SYS_TIME = 3,
} syscall_t;

typedef struct {
    Registers regs;
    RAM ram;

    uint32_t pc;
    uint8_t flags;

    int running;
} CPU;

int cpu_init(CPU* cpu, uint64_t ram_size);

void cpu_free(CPU* cpu);

fn run(CPU* cpu);

#endif
