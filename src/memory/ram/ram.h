#ifndef WVM_RAM_H
#define WVM_RAM_H

#include <stdint.h>
#include <stddef.h>
#include <assert.h>

typedef struct {
    uint8_t* data;
    uint64_t size;
} RAM;

static inline uint8_t mem_read8(const RAM* ram, const uint64_t addr) {
    return ram->data[addr];
}

static inline void mem_write8(const RAM* ram, const uint64_t addr, const uint8_t val) {
    ram->data[addr] = val;
}

static inline int64_t mem_read64(const RAM* ram, const uint64_t addr) {
    return *(int64_t*)&ram->data[addr];
}

static inline void mem_write64(const RAM* ram, const uint64_t addr, const int64_t val) {
    *(int64_t*)&ram->data[addr] = val;
}

#endif
