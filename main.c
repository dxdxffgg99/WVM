#include "src/wvm.h"
#include "src/asm/assembler.h"
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

CPU cpu;

static bool noAnsi = false;
static bool minimal = false;

#define ANSI_RED() (noAnsi ? "" : "\x1b[91m\x1b[1m")
#define ANSI_GREEN() (noAnsi ? "" : "\x1b[92m\x1b[1m")
#define ANSI_YELLOW() (noAnsi ? "" : "\x1b[93m\x1b[1m")
#define ANSI_CYAN() (noAnsi ? "" : "\x1b[96m")
#define ANSI_RESET() (noAnsi ? "" : "\x1b[0m")

static double
get_time_sec(struct timespec ts)
{
    return (double) ts.tv_sec + (double) ts.tv_nsec / 1000000000.0;
}

char *
read_assembly_file(const char *filePath, size_t *outSize)
{
    FILE *file = fopen(filePath, "rb");
    if (!file) {
        fprintf(stderr, "%s✗ Error: Cannot open file '%s'%s\n", ANSI_RED(), filePath, ANSI_RESET());
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (fileSize <= 0) {
        fprintf(stderr, "%s✗ Error: File '%s' is empty or invalid%s\n", ANSI_RED(), filePath, ANSI_RESET());
        fclose(file);
        return NULL;
    }

    char *buffer = (char *) malloc(fileSize + 1);
    if (!buffer) {
        fprintf(stderr, "%s✗ Error: Memory allocation failed%s\n", ANSI_RED(), ANSI_RESET());
        fclose(file);
        return NULL;
    }

    size_t readSize = fread(buffer, 1, fileSize, file);
    if (readSize != (size_t)fileSize) {
        fprintf(stderr, "%s✗ Error: Failed to read file '%s'%s\n", ANSI_RED(), filePath, ANSI_RESET());
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[fileSize] = '\0';
    fclose(file);

    if (outSize) {
        *outSize = fileSize;
    }

    return buffer;
}

void
print_usage(const char *programName)
{
    printf("%sUsage:%s %s <assembly_file.asm>\n", ANSI_CYAN(), ANSI_RESET(), programName);
    printf("%sExample:%s\n", ANSI_CYAN(), ANSI_RESET());
    printf("  %s program.asm\n", programName);
}

char *
get_filename_from_user(void)
{
    static char fileName[256];

    printf("%sWVM - Warp speed Virtual Machine%s\n", ANSI_GREEN(), ANSI_RESET());
    printf("%s━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┫%s\n", ANSI_CYAN(), ANSI_RESET());
    printf("%sAssembly file not specified.%s\n\n", ANSI_YELLOW(), ANSI_RESET());
    printf("%sEnter assembly file path:%s ", ANSI_RESET(), ANSI_RESET());

    if (fgets(fileName, sizeof(fileName), stdin) == NULL) {
        fprintf(stderr, "%s✗ Error: Failed to read input%s\n", ANSI_RED(), ANSI_RESET());
        return NULL;
    }

    size_t len = strlen(fileName);
    if (len > 0 && fileName[len - 1] == '\n') {
        fileName[len - 1] = '\0';
    }

    if (strlen(fileName) == 0) {
        fprintf(stderr, "%s✗ Error: Filename cannot be empty%s\n", ANSI_RED(), ANSI_RESET());
        return NULL;
    }

    return fileName;
}

int
main(int argc, char *argv[])
{
    const char *filePath = NULL;

    int arg_index = 1;
    while (arg_index < argc) {
        if (strcmp(argv[arg_index], "--help") == 0 || strcmp(argv[arg_index], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[arg_index], "--no-ansi") == 0) {
            noAnsi = true;
            arg_index++;
        } else if (strcmp(argv[arg_index], "--minimal") == 0) {
            minimal = true;
            noAnsi = true;
            arg_index++;
        } else if (argv[arg_index][0] != '-') {
            filePath = argv[arg_index];
            break;
        } else {
            arg_index++;
        }
    }

    if (!filePath) {
        if (!minimal) {
            filePath = get_filename_from_user();
        } else {
            print_usage(argv[0]);
            return 1;
        }
        if (!filePath) {
            return 1;
        }
    }

    size_t fileSize = 0;
    char *assembly = read_assembly_file(filePath, &fileSize);

    if (!assembly) {
        return 1;
    }

    if (cpu_init(&cpu, 4096)) {
        fprintf(stderr, "%s✗ Error: CPU initialization failed%s\n", ANSI_RED(), ANSI_RESET());
        free(assembly);
        return 1;
    }

    uint8_t program[4096];
    struct timespec start, end;

    if (!minimal) {
        printf("%s┌─═══[Assemble start ]═══─┐\n%s", ANSI_GREEN(), ANSI_RESET());
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
        size_t size = assemble(assembly, program, sizeof(program));
    clock_gettime(CLOCK_MONOTONIC, &end);
    double assembleTime = get_time_sec(end) - get_time_sec(start);

    if (size == 0) {
        if (minimal) {
            fprintf(stderr, "Assemble failed\n");
        } else {
            fprintf(stderr, "%s└─═══[Assemble fail]═══─┘%s\n", ANSI_RED(), ANSI_RESET());
        }
        cpu_free(&cpu);
        free(assembly);
        return 1;
    }

    if (!minimal) {
        printf("Assemble Success\n");
        printf("%s└─═══[Assemble finish]═══─┘%s\n\n", ANSI_GREEN(), ANSI_RESET());
    }

    load_program(&cpu, program, size);

    if (!minimal) {
        printf("%s┌─═══[VM start   ]═══─┐%s\n", ANSI_GREEN(), ANSI_RESET());
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
        const int64_t returnValue = run(&cpu);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double runTime = get_time_sec(end) - get_time_sec(start);

    if (!minimal) {
        printf("%s└─═══[VM finished]═══─┘%s\n\n", ANSI_GREEN(), ANSI_RESET());
        printf("%s┌─═══[Performance Report]═══─┐%s\n", ANSI_CYAN(), ANSI_RESET());
        printf("%s│ Assembly time: %s%.6f s%s  │%s\n", ANSI_CYAN(), ANSI_YELLOW(), assembleTime, ANSI_CYAN(), ANSI_RESET());
        printf("%s│ VM Run time:   %s%.6f s%s  │%s\n", ANSI_CYAN(), ANSI_YELLOW(), runTime, ANSI_CYAN(), ANSI_RESET());
        printf("%s│ Return Value:  %s%ld%s           │%s\n", ANSI_CYAN(), ANSI_GREEN(), returnValue, ANSI_CYAN(), ANSI_RESET());
        printf("%s└─══════════════════════════─┘%s\n", ANSI_CYAN(), ANSI_RESET());
    }

    cpu_free(&cpu);
    free(assembly);

    return 0;
}
