#ifndef WVM_REGISTER_H
#define WVM_REGISTER_H

#include <stdint.h>

#define REG_ZERO 64
#define REG_COUNT 64

typedef struct {
    int64_t r[REG_COUNT];
    uint64_t zf;
    uint64_t sf;
    uint64_t cf;
    uint64_t sp;
} Registers;

static inline int64_t reg_read(const Registers* regs, uint64_t reg) {
    if (reg == REG_ZERO) { return 0; }
    if (reg >= REG_COUNT) { return 0; }

    return regs->r[reg];
}

static inline void reg_write(Registers* regs, uint64_t reg, int64_t value) {
    if (reg == REG_ZERO) { return; }

    if (reg < REG_COUNT)
        regs->r[reg] = value;
}

#endif
