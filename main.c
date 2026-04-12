#include <string.h>
#include <stdio.h>
#include "src/cpu/cpu.h"

#define _8bit(v) program[i++] = (uint8_t)(v);
#define _64bit(v)                          \
(uint8_t)( (uint64_t)(v) & 0xFF ),         \
(uint8_t)( ((uint64_t)(v) >> 8) & 0xFF ),  \
(uint8_t)( ((uint64_t)(v) >> 16) & 0xFF ), \
(uint8_t)( ((uint64_t)(v) >> 24) & 0xFF ), \
(uint8_t)( ((uint64_t)(v) >> 32) & 0xFF ), \
(uint8_t)( ((uint64_t)(v) >> 40) & 0xFF ), \
(uint8_t)( ((uint64_t)(v) >> 48) & 0xFF ), \
(uint8_t)( ((uint64_t)(v) >> 56) & 0xFF )
#define r(v) v
#define rZ 64

CPU cpu;

void load_program(CPU* cpu, const uint8_t* code, size_t size) {
    memcpy(cpu->ram.data, code, size);
    cpu->pc = 0;
}

int main(void) {
    // 64KB RAM 할당
    if (cpu_init(&cpu, 65536) != 0) {
        printf("CPU initialization failed\n");
        return 1;
    }

    uint8_t program[] = {
        ADDI, r(0), r(0),
        _64bit(42),

        ADDI, r(0), r(0),
        _64bit(42),

        EOP
    };

    load_program(&cpu, program, sizeof(program));
    run(&cpu);
    printf("program data (hex):\n");
    for (int i = 0; i < sizeof(program); i++) {
        printf("%02X ", cpu.ram.data[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");

    printf("reg data (hex):\n");
    for (int i = 0; i < 64; i++) {
        printf("%ld ", cpu.regs.r[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");

    cpu_free(&cpu);
    return 0;
}

