#include "src/wvm.h"

CPU cpu;

int main(void) {
    if (cpu_init(&cpu, 64)) {
        return 1;
    }

    const uint8_t program[] = {
        ADDI, r(0), rZ,   _64bit(90),
        SUBI, r(1), r(0), _64bit(30),
        DIVI, r(2), r(1), _64bit(15),
        MULI, r(3), r(2), _64bit(5),
        TIME, r(63),
        RAND, r(4), r(63),
        DEBUG,
        NOP,
        EOP
    };

    load_program(&cpu, program, sizeof(program));
    run(&cpu);
    cpu_free(&cpu);

    return 0;
}