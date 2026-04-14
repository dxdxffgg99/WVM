#ifndef WVM_REGISTER_H
#define WVM_REGISTER_H

#include <stdint.h>

#define REG_ZERO 256
#define REG_COUNT 256

#define r(v) v /* for Indicates that it is a register */
#define rZ 256 /* Zero registor num */

typedef struct {
    int64_t registor[REG_COUNT];
    uint64_t zero_flag;
    uint64_t sign_flag;
    uint64_t carry_flag;
    uint64_t stack_pointer;
} Registers;

static inline int64_t reg_read(const Registers* regs, uint64_t reg) {
    if (reg == REG_ZERO) { return 0; }
    if (reg >= REG_COUNT) { return 0; }

    return regs->registor[reg];
}

static inline void reg_write(Registers* regs, uint64_t reg, int64_t value) {
    if (reg == REG_ZERO) { return; }
    if (reg < REG_COUNT) { regs->registor[reg] = value; }
}

typedef uint8_t register_addr_t;

#endif
