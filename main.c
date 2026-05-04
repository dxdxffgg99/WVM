#include "src/wvm.h"
#include "src/asm/assembler.h"
#include <stdio.h>
#include <time.h>

CPU cpu;

static double get_time_sec(struct timespec ts) {
    return (double) ts.tv_sec + (double) ts.tv_nsec / 1000000000.0;
}

int main(void) {
    if (cpu_init(&cpu, 4096)) {
        return 1;
    }

    const char *assembly =
            "mov $0, %r0\n"
            "mov $0, %r1\n"
            "mov $1000000000, %r2\n"
            "loop:\n"
            "  add $1, %r0\n"
            "  mul %r0, $2\n"
            "  sub %r0, $1\n"
            "  and %r0, $0xFF\n"
            "  add $1, %r1\n"
            "  cmp %r1, %r2\n"
            "  jl loop\n"
            "eopv %r0\n";

    uint8_t program[4096];
    struct timespec start, end;

    printf("Assemble start\n");

    clock_gettime(CLOCK_MONOTONIC, &start);
    size_t size = assemble(assembly, program, sizeof(program));
    clock_gettime(CLOCK_MONOTONIC, &end);
    double assemble_time = get_time_sec(end) - get_time_sec(start);

    if (size == 0) {
        printf("Assembly failed\n");
        cpu_free(&cpu);
        return 1;
    }

    printf("Assemble finished\n");

    load_program(&cpu, program, size);

    printf("VM start\n");

    clock_gettime(CLOCK_MONOTONIC, &start);
    const int64_t rv = run(&cpu);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double run_time = get_time_sec(end) - get_time_sec(start);

    printf("VM finished\n");

    printf("Assembly time: %.6f s\n", assemble_time);
    printf("VM Run time:   %.6f s\n", run_time);
    printf("RV: %ld\n", rv);

    cpu_free(&cpu);

    return 0;
}
