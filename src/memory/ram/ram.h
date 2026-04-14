#ifndef WVM_RAM_H
#define WVM_RAM_H

#include <stdint.h>
#include <assert.h>

typedef struct {
    uint8_t* data;
    uint64_t size;
} RAM;

static inline uint8_t
mem_read8(
    const RAM* ram,
    const uint64_t addr)
{
    assert(addr < ram->size);
    return ram->data[addr];
}

static inline void
mem_write8(const RAM* ram,
           const uint64_t addr,
           const uint8_t val)
{
    assert(addr < ram->size);
    ram->data[addr] = val;
}

static inline int64_t
mem_read64(
    const RAM* ram,
    const uint64_t addr)
{
    assert(addr + 7 < ram->size);

    uint64_t v = 0;

    for (int i = 0; i < 8; i++) {
        v |= ((uint64_t)ram->data[addr + i]) << (8 * i);
    }

    return (int64_t)v;
}

static inline void
mem_write64(const RAM* ram,
    const uint64_t addr,
    const int64_t val)
{
    assert(addr + 7 < ram->size);

    uint64_t v = (uint64_t)val;

    for (int i = 0; i < 8; i++) {
        ram->data[addr + i] = (uint8_t)(v >> (8 * i));
    }
}

#endif
