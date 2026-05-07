#ifndef WVM_ASSEMBLER_H
#define WVM_ASSEMBLER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../bytecode/opcode.h"

typedef enum {
    ASM_ERROR_NONE = 0,
    ASM_ERROR_INVALID_OPCODE = 1,
    ASM_ERROR_INVALID_REGISTER = 2,
    ASM_ERROR_INVALID_IMMEDIATE = 3,
    ASM_ERROR_UNDEFINED_SYMBOL = 4,
    ASM_ERROR_SYMBOL_REDEFINED = 5,
    ASM_ERROR_INVALID_SYNTAX = 6,
    ASM_ERROR_BUFFER_OVERFLOW = 7,
    ASM_ERROR_MAX_SYMBOLS_EXCEEDED = 8,
} AsmErrorType;

typedef struct {
    AsmErrorType type;
    int lineNumber;
    char message[256];
    char problematicLine[512];
} AsmError;

typedef struct {
    AsmError lastError;
    int totalLines;
    int errorCount;
    bool debugMode;
} AsmContext;

typedef struct {
    const char *name;
    opcode_t op;
} OpMapping;

AsmContext *asm_create_context(bool debug_mode);

void asm_free_context(AsmContext *ctx);

const char *asm_get_error_message(AsmContext *ctx);

const char *asm_get_error_type(AsmErrorType type);

size_t asm_assemble(AsmContext *ctx, const char *source, uint8_t *output, size_t max_size);

size_t assemble(const char *source, uint8_t *output, size_t maxSize);

#endif
