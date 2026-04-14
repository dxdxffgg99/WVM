#include "src/wvm.h"

CPU cpu;

int main(void) {
    if (cpu_init(&cpu, 1024)) { return 1; }

    const instr_t program[] = {
        { MOV, 1, r(0), 0, 0, 100000000 },
        { MOV, 1, r(1), 0, 0, 0 },

        { ADD, 0, r(1), r(0), 0, 0 },
        { XOR, 1, r(1), r(1), 0, 1234567 },
        { ADD, 1, r(0), 0, 0, -1 },
        { CMP, 1, r(0), 0, 0, 0 },
        { JG, 1, 0, 0, 0, 26 },

        { EOP, 0, 0, 0, 0, 0 },,,
    };

    load_program(&cpu, program, sizeof(program) / sizeof(instr_t));

    const int rv = (int)run(&cpu);

    return rv;
}