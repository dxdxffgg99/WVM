#ifndef WVM_CPU_H
#define WVM_CPU_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "../memory/ram/ram.h"
#include "../memory/register/register.h"
#include "../bytecode/opcode.h"

#define get_src1 (ins->mode) ? ins->imm: reg_read(&cpu->regs, ins->src1)
#define get_src2 (ins->mode) ? ins->imm: reg_read(&cpu->regs, ins->src2)

typedef int64_t  imm_t;
typedef uint8_t  reg_addr_t;
typedef uint64_t ram_addr_t;

typedef struct {
    Registers regs;
    RAM ram;
    ram_addr_t pc;
    bool running;
    int rv;
} CPU;

typedef struct __attribute__((packed)) {
    uint8_t opcode;
    uint8_t mode;
    uint8_t dst;
    uint8_t src1;
    uint8_t src2;
    int64_t imm;
} instr_t;

static inline void
instr_encode(const instr_t* instr, uint8_t* buffer) {

}

static inline void
instr_decode(const uint8_t* buffer, instr_t* instr) {
    instr->opcode = buffer[0];
    instr->mode = buffer[1];
    instr->dst = buffer[2];
    instr->src1 = buffer[3];
    instr->src2 = buffer[4];

    instr->imm = 0;
    for (int i = 0; i < 8; i++) {
        instr->imm |= ((imm_t)buffer[5 + i]) << (8 * i);
    }
}

static inline ram_addr_t
instr_array_size(const ram_addr_t instr_count) {
    return instr_count * 13;
}

static inline void
instr_array_encode(const instr_t* instrs, uint8_t* buffer, const ram_addr_t instr_count) {

}

static bool
cpu_init(CPU* cpu, const ram_addr_t ram_size) {
    if (!(cpu && ram_size)) { return 1; }

    cpu->ram.data = (uint8_t*)malloc(ram_size);
    if (!cpu->ram.data) { return 1; }

    cpu->ram.size = ram_size;
    memset(cpu->ram.data, 0, ram_size);

    memset(&cpu->regs, 0, sizeof(Registers));
    cpu->regs.stack_pointer = ram_size;

    cpu->pc = 0;
    cpu->running = 0;

    return 0;
}

static inline void
load_program(CPU* cpu, const instr_t* code, const ram_addr_t instr_count) {
    for (ram_addr_t i = 0; i < instr_count; i++) {
        uint8_t* target = &cpu->ram.data[i * 13];

        target[0] = code[i].opcode;
        target[1] = code[i].mode;
        target[2] = code[i].dst;
        target[3] = code[i].src1;
        target[4] = code[i].src2;

        for (int j = 0; j < 8; j++) {
            target[5 + j] = (uint8_t)((code[i].imm >> (8 * j)) & 0xFF);
        }
    }
    cpu->pc = 0;
}

uint64_t run(CPU* cpu);

#endif
