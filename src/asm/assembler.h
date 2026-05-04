#ifndef WVM_ASSEMBLER_H
#define WVM_ASSEMBLER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

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
        int line_number;
        char message[256];
        char problematic_line[512];
} AsmError;

typedef struct {
        AsmError last_error;
        int total_lines;
        int error_count;
        bool debug_mode;
} AsmContext;

AsmContext *asm_create_context(bool debug_mode);

void asm_free_context(AsmContext *ctx);

const char *asm_get_error_message(AsmContext *ctx);

const char *asm_get_error_type(AsmErrorType type);

size_t asm_assemble(AsmContext *ctx, const char *source, uint8_t *output, size_t max_size);

size_t assemble(const char *source, uint8_t *output, size_t max_size);

#endif
