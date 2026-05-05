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
        fprintf(stderr, "\x1b[1mError: Cannot open file '%s'\n\x1b[0m", filepath);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size <= 0) {
        fprintf(stderr, "\x1b[1mError: File '%s' is empty or invalid\n\x1b[0m", filepath);
        fclose(file);
        return NULL;
    }

    char *buffer = (char *) malloc(file_size + 1);
    if (!buffer) {
        fprintf(stderr, "\x1b[1mError: Memory allocation failed\n\x1b[0m");
        fclose(file);
        return NULL;
    }

    size_t read_size = fread(buffer, 1, file_size, file);
    if (read_size != (size_t) file_size) {
        fprintf(stderr, "\x1b[1mError: Failed to read file '%s'\n\x1b[0m", filepath);
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
    printf("\x1b[96m\x1b[1mUsage:\x1b[0m %s <assembly_file.asm>\n", program_name);
    printf("\x1b[96m\x1b[1mExample:\x1b[0m\n");
    printf("  %s program.asm\n", program_name);
}

char *
get_filename_from_user() {
    static char filename[256];

    printf("\x1b[92m\x1b[1mWVM - WebAssembly Virtual Machine\x1b[0m\n");
    printf("\x1b[96m━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\x1b[0m\n");
    printf("\x1b[93m\x1b[1mAssembly file not specified.\x1b[0m\n\n");
    printf("\x1b[1mEnter assembly file path:\x1b[0m ");

    if (fgets(filename, sizeof(filename), stdin) == NULL) {
        fprintf(stderr, "\x1b[1mError: Failed to read input\n\x1b[0m");
        return NULL;
    }

    // Remove newline character
    size_t len = strlen(filename);
    if (len > 0 && filename[len - 1] == '\n') {
        filename[len - 1] = '\0';
    }

    // Check if filename is empty
    if (strlen(filename) == 0) {
        fprintf(stderr, "\x1b[1mError: Filename cannot be empty\n\x1b[0m");
        return NULL;
    }

    return filename;
}

int
main(int argc, char *argv[]) {
    const char *filepath;

    if (argc < 2) {
        filepath = get_filename_from_user();
        if (!filepath) {
            print_usage(argv[0]);
            return 1;
        }
    } else {
        filepath = argv[1];
    }
    size_t file_size = 0;
    char *assembly = read_assembly_file(filepath, &file_size);

    if (!assembly) {
        return 1;
    }

    if (cpu_init(&cpu, 4096)) {
        fprintf(stderr, "\x1b[1mError: CPU initialization failed\n\x1b[0m");
        free(assembly);
        return 1;
    }

    uint8_t program[4096];
    struct timespec start, end;

    printf("\x1b[92m\x1b[1m===[Assemble start]===\n\x1b[0m");

    clock_gettime(CLOCK_MONOTONIC, &start);
    size_t size = assemble(assembly, program, sizeof(program));
    clock_gettime(CLOCK_MONOTONIC, &end);
    double assemble_time = get_time_sec(end) - get_time_sec(start);

    if (size == 0) {
        fprintf(stderr, "\x1b[1m===[Assembly failed]===\n\x1b[0m");
        cpu_free(&cpu);
        free(assembly);
        return 1;
    }

    printf("\x1b[92m\x1b[1m===[Assemble finished]===\n\x1b[0m");

    load_program(&cpu, program, size);

    printf("\x1b[92m\x1b[1m===[VM start]===\n\x1b[0m");

    clock_gettime(CLOCK_MONOTONIC, &start);
    const int64_t rv = run(&cpu);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double run_time = get_time_sec(end) - get_time_sec(start);

    printf("\x1b[92m\x1b[1m===[VM finished]===\n\x1b[0m");

    printf("\x1b[96mAssembly time: \x1b[0m%.6f s\n", assemble_time);
    printf("\x1b[96mVM Run time:   \x1b[0m%.6f s\n", run_time);
    printf("\x1b[96mRV: \x1b[0m%ld\n", rv);

    cpu_free(&cpu);
    free(assembly);

    return 0;
}
