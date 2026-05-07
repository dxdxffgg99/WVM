#ifndef WVM_CPU_H
#define WVM_CPU_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "../memory/ram/ram.h"
#include "../memory/register/register.h"

#define ADDR_MODE_IMM  0x01
#define ADDR_MODE_IMM8 0x02

typedef int64_t imm_t;
typedef uint8_t reg_addr_t;
typedef uint64_t ram_addr_t;

typedef struct instr_s {
    uint8_t opcode;
    uint8_t mode;
    uint8_t dst;
    uint8_t src1;
    uint8_t src2;
    int64_t imm;
    uint8_t size;
    uint64_t addr;
    int32_t jump_pci;
    void *handler;
    struct instr_s *jump_target;
} instr_t;

typedef struct {
    Registers regs;
    RAM ram;
    ram_addr_t pc;
    instr_t *decodedProgram;
    ram_addr_t decodedSize;
    bool running;
    bool isThreaded;
    int64_t returnValue;
} CPU;

#define INSTR_MIN_SIZE 5
#define INSTR_MAX_SIZE 13

bool instr_decode(const uint8_t *buffer, size_t limit, instr_t *instr);

ram_addr_t instr_array_size( ram_addr_t instr_count);

int cpu_init(CPU *cpu, ram_addr_t ram_size);

void cpu_free(CPU *cpu);

void load_program(CPU *cpu, const uint8_t *code, ram_addr_t size);

void cpu_dump_registers(const CPU *cpu);

int64_t run(CPU *cpu);

#endif
