#ifndef WVM_REGISTER_H
#define WVM_REGISTER_H

#include <stdint.h>

#define REG_COUNT 256

#define r(v) (v)    /* Indicates that it is a register index */
#define rZ    (256)  /* Zero register number */

typedef struct {
    int64_t registers[REG_COUNT];
    uint64_t zero_flag;
    uint64_t sign_flag;
    uint64_t carry_flag;
    uint64_t stack_pointer;
} Registers;

static inline int64_t reg_read(const Registers *regs, const uint64_t reg) {
    return regs->registers[reg];
}

static inline void reg_write(Registers *regs, const uint64_t reg, const int64_t value) {
    regs->registers[reg] = value;
}

#endif
