#include "src/wvm.h"
#include "src/asm/assembler.h"
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

CPU cpu;

static double
get_time_sec(struct timespec ts) {
    return (double) ts.tv_sec + (double) ts.tv_nsec / 1000000000.0;
}

char *
read_assembly_file(const char *filepath, size_t *out_size) {
    FILE *file = fopen(filepath, "rb");
    if (!file) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", filepath);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size <= 0) {
        fprintf(stderr, "Error: File '%s' is empty or invalid\n", filepath);
        fclose(file);
        return NULL;
    }

    char *buffer = (char *) malloc(file_size + 1);
    if (!buffer) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(file);
        return NULL;
    }

    size_t read_size = fread(buffer, 1, file_size, file);
    if (read_size != (size_t) file_size) {
        fprintf(stderr, "Error: Failed to read file '%s'\n", filepath);
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[file_size] = '\0';
    fclose(file);

    if (out_size) {
        *out_size = file_size;
    }

    return buffer;
}

void
print_usage(const char *program_name) {
    printf("Usage: %s <assembly_file.asm>\n", program_name);
    printf("\nExample:\n");
    printf("  %s program.asm\n", program_name);
}

int
main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Error: No assembly file specified\n");
        print_usage(argv[0]);
        return 1;
    }

    const char *filepath = argv[1];
    size_t file_size = 0;
    char *assembly = read_assembly_file(filepath, &file_size);

    if (!assembly) {
        return 1;
    }

    if (cpu_init(&cpu, 4096)) {
        fprintf(stderr, "Error: CPU initialization failed\n");
        free(assembly);
        return 1;
    }

    uint8_t program[4096];
    struct timespec start, end;

    printf("===[Assemble start]===\n");

    clock_gettime(CLOCK_MONOTONIC, &start);
    size_t size = assemble(assembly, program, sizeof(program));
    clock_gettime(CLOCK_MONOTONIC, &end);
    double assemble_time = get_time_sec(end) - get_time_sec(start);

    if (size == 0) {
        fprintf(stderr, "===[Assembly failed]===\n");
        cpu_free(&cpu);
        free(assembly);
        return 1;
    }

    printf("===[Assemble finished]===\n");

    load_program(&cpu, program, size);

    printf("===[VM start]===\n");

    clock_gettime(CLOCK_MONOTONIC, &start);
    const int64_t rv = run(&cpu);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double run_time = get_time_sec(end) - get_time_sec(start);

    printf("===[VM finished]===\n");

    printf("Assembly time: %.6f s\n", assemble_time);
    printf("VM Run time:   %.6f s\n", run_time);
    printf("RV: %ld\n", rv);

    cpu_free(&cpu);
    free(assembly);

    return 0;
}
