#include "assembler.h"

static const OpMapping op_table[] = {
    {"nop", NOP},
    {"add", ADD},
    {"sub", SUB},
    {"mul", MUL},
    {"div", DIV},
    {"inc", INC},
    {"dec", DEC},
    {"jmp", JMP},
    {"cmp", CMP},
    {"je", JE},
    {"jne", JNE},
    {"jl", JL},
    {"jg", JG},
    {"jle", JLE},
    {"jge", JGE},
    {"setz", SETZ},
    {"mov", MOV},
    {"lsh", LSH},
    {"rsh", RSH},
    {"load", LOAD},
    {"store", STORE},
    {"or", OR},
    {"and", AND},
    {"xor", XOR},
    {"time", TIME},
    {"push", PUSH},
    {"pop", POP},
    {"call", CALL},
    {"ret", RET},
    {"eop", EOP},
    {"eopv", EOPV},
    {"debug", DEBUG},
    {"loop", LOOP},
    {"syscall", SYSCALL},
    {NULL, NOP}
};

typedef struct {
    char name[64];
    uint64_t addr;
} Symbol;

#define MAX_SYMBOLS 1024
static Symbol symbol_table[MAX_SYMBOLS];
static int symbol_count = 0;

static const char *error_type_strings[] = {
    "ASM_ERROR_NONE",
    "ASM_ERROR_INVALID_OPCODE",
    "ASM_ERROR_INVALID_REGISTER",
    "ASM_ERROR_INVALID_IMMEDIATE",
    "ASM_ERROR_UNDEFINED_SYMBOL",
    "ASM_ERROR_SYMBOL_REDEFINED",
    "ASM_ERROR_INVALID_SYNTAX",
    "ASM_ERROR_BUFFER_OVERFLOW",
    "ASM_ERROR_MAX_SYMBOLS_EXCEEDED",
};

/**
 * @brief Sets an error in the assembler context.
 *
 * This function records an error with the specified type, line number, and message
 * in the provided assembler context. It also increments the error count.
 *
 * @param ctx Pointer to the AsmContext structure. If NULL, the function returns early.
 * @param type The type of error to set.
 * @param line_number The line number where the error occurred.
 * @param line_text The text of the problematic line, or NULL if not available.
 * @param fmt Format string for the error message, followed by variable arguments.
 */
static void
set_error(AsmContext *ctx,
          AsmErrorType type,
          int line_number,
          const char *line_text,
          const char *fmt, ...) {
    if (!ctx) {
        return;
    }

    ctx->last_error.type = type;
    ctx->last_error.line_number = line_number;

    if (line_text) {
        strncpy(ctx->last_error.problematic_line, line_text, sizeof(ctx->last_error.problematic_line) - 1);
        ctx->last_error.problematic_line[sizeof(ctx->last_error.problematic_line) - 1] = '\0';
    } else {
        ctx->last_error.problematic_line[0] = '\0';
    }

    va_list args;
    va_start(args, fmt);
    vsnprintf(ctx->last_error.message, sizeof(ctx->last_error.message), fmt, args);
    va_end(args);

    ctx->error_count++;
}

/**
 * @brief Creates a new assembler context.
 *
 * This function allocates and initializes a new AsmContext structure with default values.
 * It sets up the symbol table, bytecode buffer, and other necessary components for assembly.
 * The debug_mode parameter controls whether debug information is included.
 *
 * @param debug_mode If true, enables debug mode for additional logging and checks.
 * @return Pointer to the newly created AsmContext, or NULL if allocation fails.
 */
AsmContext *
asm_create_context(bool debug_mode) {
    AsmContext *ctx = (AsmContext *) malloc(sizeof(AsmContext));

    if (!ctx) {
        return NULL;
    }

    memset(ctx, 0, sizeof(AsmContext));
    ctx->debug_mode = debug_mode;

    return ctx;
}

/**
 * @brief Frees an assembler context.
 *
 * This function deallocates the memory associated with the given AsmContext,
 * including the symbol table and bytecode buffer.
 *
 * @param ctx Pointer to the AsmContext to free. If NULL, the function does nothing.
 */
void
asm_free_context(AsmContext *ctx) {
    if (ctx) {
        free(ctx);
    }
}

/**
 * @brief Gets the last error message from the assembler context.
 *
 * This function returns the message of the last error that occurred in the assembler context.
 *
 * @param ctx Pointer to the AsmContext. If NULL, returns NULL.
 * @return The error message string, or NULL if no context is provided.
 */
const char *
asm_get_error_message(AsmContext *ctx) {
    if (!ctx) {
        return NULL;
    }

    return ctx->last_error.message;
}

/**
 * @brief Gets the string representation of an error type.
 *
 * This function converts an AsmErrorType enum value to its corresponding string representation.
 *
 * @param type The error type to convert.
 * @return String representation of the error type, or "UNKNOWN" if invalid.
 */
const char *
asm_get_error_type(AsmErrorType type) {
    if (type < 0 ||
        type >= (int) (sizeof(error_type_strings) / sizeof(error_type_strings[0]))) {
        return "UNKNOWN";
    }

    return error_type_strings[type];
}

/**
 * @brief Trims whitespace from both ends of a string.
 *
 * This function removes leading and trailing whitespace characters from the input string.
 * The string is modified in-place.
 *
 * @param str The string to trim. If NULL, returns NULL.
 * @return Pointer to the trimmed string (same as input).
 */
static char *
trim(char *str) {
    if (!str) {
        return str;
    }

    while (isspace((unsigned char) *str)) {
        str++;
    }

    if (*str == '\0') {
        return str;
    }

    char *end = str + strlen(str) - 1;

    while (end > str && isspace((unsigned char) *end)) {
        end--;
    }

    end[1] = '\0';

    return str;
}

/**
 * @brief Parses a register string into a register number.
 *
 * This function parses a string in the format "%rN" where N is the register number.
 * It validates the format and ensures the register number is within valid range.
 *
 * @param str The string to parse (expected format: "%rN").
 * @return The register number (0-based), or -1 if parsing fails.
 */
static int
parse_register(const char *str) {
    if (!str ||
        str[0] != '%' ||
        str[1] != 'r') {
        return -1;
    }

    char *end = NULL;

    long value = strtol(&str[2], &end, 10);

    if (end == &str[2] ||
        *end != '\0' ||
        value < 0 ||
        value >= REG_COUNT) {
        return -1;
    }

    return (int) value;
}

/**
 * @brief Parses an immediate value string.
 *
 * This function parses a string in the format "$VALUE" where VALUE is a numeric constant.
 * It supports decimal, hexadecimal (0x prefix), and octal (0 prefix) formats.
 *
 * @param str The string to parse (expected format: "$VALUE").
 * @param value Pointer to store the parsed value.
 * @param ctx Pointer to the assembler context for error reporting.
 * @param line_number The line number for error reporting.
 * @return true if parsing succeeds, false otherwise.
 */
static bool
parse_immediate(const char *str,
                int64_t *value,
                AsmContext *ctx,
                int line_number) {
    if (!str ||
        str[0] != '$') {
        return false;
    }

    char *end = NULL;
    errno = 0;
    long long result = strtoll(&str[1], &end, 0);

    if (end == &str[1] ||
        *end != '\0' ||
        (errno == ERANGE &&
         (result == LLONG_MAX ||
          result == LLONG_MIN))) {
        set_error(ctx, ASM_ERROR_INVALID_IMMEDIATE, line_number, str, "Invalid immediate value '%s'", str);
        return false;
    }
    *value = (int64_t) result;

    return true;
}

/**
 * @brief Splits a comma-separated operand string into individual operands.
 *
 * This function parses a string containing operands separated by commas and stores
 * them in the provided operands array. It validates the syntax and trims whitespace.
 *
 * @param input The input string containing operands (modified in-place).
 * @param operands Array to store the parsed operands (up to 3 operands, each up to 64 chars).
 * @param count Pointer to store the number of operands found.
 * @param ctx Pointer to the assembler context for error reporting.
 * @param line_number The line number for error reporting.
 * @return true if parsing succeeds, false otherwise.
 */
static bool
split_operands(char *input,
               char operands[3][64],
               int *count,
               AsmContext *ctx,
               int line_number) {
    *count = 0;
    char *p = input;

    while (*p && *count < 3) {
        while (isspace((unsigned char) *p)) {
            p++;
        }

        if (*p == '\0') {
            break;
        }

        int idx = 0;

        while (*p != '\0' &&
               *p != ',' &&
               idx < 63) {
            operands[*count][idx++] = *p++;
        }

        operands[*count][idx] = '\0';
        if (*p == ',') p++;

        (*count)++;
    }

    if (*count == 0) {
        return true;
    }

    while (isspace((unsigned char) *p)) {
        p++;
    }

    if (*p != '\0') {
        set_error(ctx, ASM_ERROR_INVALID_SYNTAX, line_number, input, "Too many operands in '%s'", input);
        return false;
    }

    for (int i = 0; i < *count; i++) {
        trim(operands[i]);

        if (operands[i][0] == '\0') {
            set_error(ctx, ASM_ERROR_INVALID_SYNTAX, line_number, input, "Empty operand in '%s'", input);
            return false;
        }
    }

    return true;
}

/**
 * @brief Gets the opcode value for a given instruction name.
 *
 * This function looks up the opcode table to find the numeric opcode value
 * corresponding to the given instruction name.
 *
 * @param name The instruction name to look up.
 * @return The opcode value, or -1 if the instruction name is not found.
 */
static int
get_opcode(const char *name) {
    for (int i = 0; op_table[i].name != NULL; i++) {
        if (strcmp(name, op_table[i].name) == 0) {
            return (int) op_table[i].op;
        }
    }

    return -1;
}

/**
 * @brief Adds a symbol to the symbol table.
 *
 * This function adds a new symbol with the given name and address to the symbol table.
 * It checks for duplicate symbols and validates the symbol name.
 *
 * @param ctx Pointer to the assembler context.
 * @param name The symbol name to add.
 * @param addr The address associated with the symbol.
 * @param line_number The line number for error reporting.
 * @return true if the symbol was added successfully, false otherwise.
 */
static bool
add_symbol(AsmContext *ctx,
           const char *name,
           uint64_t addr,
           int line_number) {
    if (!name || *name == '\0') {
        set_error(ctx, ASM_ERROR_INVALID_SYNTAX, line_number, name, "Invalid symbol name");
        return false;
    }

    if (symbol_count >= MAX_SYMBOLS) {
        set_error(ctx, ASM_ERROR_MAX_SYMBOLS_EXCEEDED, line_number, name, "Symbol table limit exceeded");
        return false;
    }

    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) {
            set_error(ctx, ASM_ERROR_SYMBOL_REDEFINED, line_number, name, "Symbol '%s' redefined", name);
            return false;
        }
    }

    strncpy(symbol_table[symbol_count].name, name, sizeof(symbol_table[symbol_count].name) - 1);
    symbol_table[symbol_count].name[sizeof(symbol_table[symbol_count].name) - 1] = '\0';
    symbol_table[symbol_count].addr = addr;
    symbol_count++;

    return true;
}

/**
 * @brief Finds a symbol in the symbol table.
 *
 * This function searches the symbol table for a symbol with the given name
 * and returns its address if found.
 *
 * @param name The symbol name to search for.
 * @return The address of the symbol, or -1 if not found.
 */
static int64_t
find_symbol(const char *name) {
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) {
            return (int64_t) symbol_table[i].addr;
        }
    }
    return -1;
}

/**
 * @brief Calculates the size of an instruction in bytes.
 *
 * This function determines the total size of an instruction based on its addressing mode,
 * including the base instruction size plus any immediate operands.
 *
 * @param mode The addressing mode of the instruction.
 * @return The size of the instruction in bytes.
 */
static size_t
instruction_size(uint8_t mode) {
    size_t size = 5;

    if (mode & ADDR_MODE_IMM) {
        size += (mode & ADDR_MODE_IMM8) ? 8 : 4;
    }

    return size;
}

/**
 * @brief Encodes an instruction into bytecode.
 *
 * This function encodes the given instruction parameters into the bytecode buffer.
 * It handles the instruction header and immediate operands if present.
 *
 * @param opcode The instruction opcode.
 * @param mode The addressing mode.
 * @param dst The destination register.
 * @param src1 The first source register.
 * @param src2 The second source register.
 * @param imm The immediate value (if applicable).
 * @param out The output bytecode buffer, or NULL to just calculate size.
 * @param pos Pointer to the current position in the buffer.
 * @param max_size The maximum size of the output buffer.
 * @return true if encoding succeeds, false if buffer overflow would occur.
 */
static bool
encode_instruction(uint8_t opcode,
                   uint8_t mode,
                   uint8_t dst,
                   uint8_t src1,
                   uint8_t src2,
                   int64_t imm,
                   uint8_t *out,
                   size_t *pos,
                   size_t max_size) {
    size_t size = instruction_size(mode);
    if (out && *pos + size > max_size) {
        return false;
    }

    if (out) {
        out[(*pos)++] = opcode;
        out[(*pos)++] = mode;
        out[(*pos)++] = dst;
        out[(*pos)++] = src1;
        out[(*pos)++] = src2;
        if (mode & ADDR_MODE_IMM) {
            int imm_size = (mode & ADDR_MODE_IMM8) ? 8 : 4;
            for (int i = 0; i < imm_size; i++) {
                out[(*pos)++] = (uint8_t) ((imm >> (8 * i)) & 0xFF);
            }
        }
    } else {
        *pos += size;
    }
    return true;
}

/**
 * @brief Resolves an operand string into its value and type.
 *
 * This function parses an operand string and determines whether it's an immediate value,
 * a register, or a symbol/label. It resolves symbols to their addresses.
 *
 * @param ctx Pointer to the assembler context.
 * @param operand The operand string to resolve.
 * @param imm Pointer to store the immediate value or resolved symbol address.
 * @param reg Pointer to store the register number.
 * @param is_imm Pointer to store whether the operand is an immediate value.
 * @param allow_label Whether labels/symbols are allowed in this context.
 * @param require_symbol Whether undefined symbols should be treated as errors.
 * @param line_number The line number for error reporting.
 * @return true if resolution succeeds, false otherwise.
 */
static bool
resolve_operand(AsmContext *ctx,
                const char *operand,
                int64_t *imm,
                int *reg,
                bool *is_imm,
                bool allow_label,
                bool require_symbol,
                int line_number) {
    *is_imm = false;
    *imm = 0;
    *reg = 0;

    if (!operand || *operand == '\0') {
        return true;
    }

    if (operand[0] == '$') {
        if (!parse_immediate(operand, imm, ctx, line_number)) {
            return false;
        }
        *is_imm = true;
        return true;
    }

    if (operand[0] == '%') {
        int value = parse_register(operand);
        if (value < 0) {
            set_error(ctx, ASM_ERROR_INVALID_REGISTER, line_number, operand, "Invalid register '%s'", operand);
            return false;
        }
        *reg = value;
        return true;
    }

    if (!allow_label) {
        set_error(ctx, ASM_ERROR_INVALID_SYNTAX, line_number, operand,
                  "Unsupported operand '%s'", operand);
        return false;
    }

    int64_t addr = find_symbol(operand);
    if (addr < 0) {
        if (require_symbol) {
            set_error(ctx, ASM_ERROR_UNDEFINED_SYMBOL, line_number, operand,
                      "Undefined symbol '%s'", operand);
            return false;
        }
        *imm = 0;
        *is_imm = true;
        return true;
    }
    *imm = addr;
    *is_imm = true;
    return true;
}

/**
 * @brief Builds the instruction layout for a given opcode and operands.
 *
 * This function analyzes the operands for a specific instruction and determines
 * the addressing mode, registers, and immediate values needed for encoding.
 *
 * @param ctx Pointer to the assembler context.
 * @param op The opcode of the instruction.
 * @param operands Array of operand strings.
 * @param count Number of operands provided.
 * @param mode Pointer to store the determined addressing mode.
 * @param dst Pointer to store the destination register.
 * @param src1 Pointer to store the first source register.
 * @param src2 Pointer to store the second source register.
 * @param imm Pointer to store the immediate value.
 * @param resolve_symbols Whether to resolve symbol references.
 * @param line_number The line number for error reporting.
 * @return true if the layout is successfully built, false otherwise.
 */
static bool
build_instruction_layout(AsmContext *ctx,
                         opcode_t op,
                         char operands[3][64],
                         int count,
                         uint8_t *mode,
                         uint8_t *dst,
                         uint8_t *src1,
                         uint8_t *src2,
                         int64_t *imm,
                         bool resolve_symbols,
                         int line_number) {
    *mode = 0;
    *dst = 0;
    *src1 = 0;
    *src2 = 0;
    *imm = 0;

    bool is_imm = false;
    int reg = 0;
    bool result = true;

    switch (op) {
        case MOV:
            if (count != 2) {
                set_error(ctx, ASM_ERROR_INVALID_SYNTAX, line_number, operands[0],
                          "MOV requires 2 operands");
                return false;
            }
            if (operands[0][0] == '$' || (operands[0][0] != '%' && operands[0][0] != '\0')) {
                result = resolve_operand(ctx, operands[0], imm, &reg, &is_imm, true, resolve_symbols, line_number);
                if (!result) {
                    return false;
                }
                if (!is_imm) {
                    set_error(ctx, ASM_ERROR_INVALID_SYNTAX, line_number, operands[0],
                              "MOV source must be immediate or register");
                    return false;
                }
                *mode = ADDR_MODE_IMM;
                *dst = (uint8_t) parse_register(operands[1]);
                if (parse_register(operands[1]) < 0) {
                    set_error(ctx, ASM_ERROR_INVALID_REGISTER, line_number, operands[1],
                              "Invalid destination register '%s'", operands[1]);
                    return false;
                }
            } else {
                if (!resolve_operand(ctx, operands[0], imm, &reg, &is_imm, false, false, line_number)) return false;
                *src1 = (uint8_t) reg;
                if (parse_register(operands[1]) < 0) {
                    set_error(ctx, ASM_ERROR_INVALID_REGISTER, line_number, operands[1],
                              "Invalid destination register '%s'", operands[1]);
                    return false;
                }
                *dst = (uint8_t) parse_register(operands[1]);
            }
            break;
        case ADD:
        case SUB:
        case MUL:
        case DIV:
        case OR:
        case AND:
        case XOR:
        case LSH:
        case RSH:
            if (count == 2) {
                if (operands[0][0] == '$' || (operands[0][0] != '%' && operands[0][0] != '\0')) {
                    result = resolve_operand(ctx, operands[0], imm, &reg, &is_imm, true, resolve_symbols, line_number);
                    if (!result) {
                        return false;
                    }
                    if (!is_imm) {
                        set_error(ctx, ASM_ERROR_INVALID_SYNTAX, line_number, operands[0],
                                  "First operand must be immediate or symbol if two operands are used");
                        return false;
                    }
                    *mode = ADDR_MODE_IMM;
                    if (parse_register(operands[1]) < 0) {
                        set_error(ctx, ASM_ERROR_INVALID_REGISTER, line_number, operands[1],
                                  "Invalid destination register '%s'", operands[1]);
                        return false;
                    }
                    *dst = (uint8_t) parse_register(operands[1]);
                    *src1 = *dst;
                } else {
                    if (parse_register(operands[0]) < 0 || parse_register(operands[1]) < 0) {
                        set_error(ctx, ASM_ERROR_INVALID_REGISTER, line_number, operands[0],
                                  "Invalid register operand");
                        return false;
                    }
                    *src2 = (uint8_t) parse_register(operands[0]);
                    *dst = (uint8_t) parse_register(operands[1]);
                    *src1 = *dst;
                }
            } else if (count == 3) {
                if (parse_register(operands[0]) < 0 || parse_register(operands[2]) < 0) {
                    set_error(ctx, ASM_ERROR_INVALID_REGISTER, line_number, operands[0],
                              "Invalid register operand");
                    return false;
                }
                *src1 = (uint8_t) parse_register(operands[0]);
                if (operands[1][0] == '$' || (operands[1][0] != '%' && operands[1][0] != '\0')) {
                    result = resolve_operand(ctx, operands[1], imm, &reg, &is_imm, true, resolve_symbols, line_number);
                    if (!result) {
                        return false;
                    }
                    if (!is_imm) {
                        set_error(ctx, ASM_ERROR_INVALID_SYNTAX, line_number, operands[1],
                                  "Expected immediate or symbol in second operand");
                        return false;
                    }
                    *mode = ADDR_MODE_IMM;
                } else {
                    if (parse_register(operands[1]) < 0) {
                        set_error(ctx, ASM_ERROR_INVALID_REGISTER, line_number, operands[1],
                                  "Invalid register operand '%s'", operands[1]);
                        return false;
                    }
                    *src2 = (uint8_t) parse_register(operands[1]);
                }
                *dst = (uint8_t) parse_register(operands[2]);
            } else {
                set_error(ctx, ASM_ERROR_INVALID_SYNTAX, line_number, operands[0],
                          "Invalid number of operands for %s", op_table[op].name);
                return false;
            }
            break;
        case INC:
        case DEC:
            if (count != 1) {
                set_error(ctx, ASM_ERROR_INVALID_SYNTAX, line_number, operands[0],
                          "%s requires 1 operand", op_table[op].name);
                return false;
            }
            if (parse_register(operands[0]) < 0) {
                set_error(ctx, ASM_ERROR_INVALID_REGISTER, line_number, operands[0],
                          "Invalid register '%s'", operands[0]);
                return false;
            }
            *dst = (uint8_t) parse_register(operands[0]);
            break;
        case JMP:
        case CALL:
            if (count != 1) {
                set_error(ctx, ASM_ERROR_INVALID_SYNTAX, line_number, operands[0],
                          "%s requires 1 operand", op_table[op].name);
                return false;
            }
            if (operands[0][0] == '%') {
                if (parse_register(operands[0]) < 0) {
                    set_error(ctx, ASM_ERROR_INVALID_REGISTER, line_number, operands[0],
                              "Invalid register '%s'", operands[0]);
                    return false;
                }
                *src1 = (uint8_t) parse_register(operands[0]);
                *src2 = *src1;
            } else {
                result = resolve_operand(ctx, operands[0], imm, &reg, &is_imm, true, resolve_symbols, line_number);
                if (!result) {
                    return false;
                }
                *mode = ADDR_MODE_IMM;
            }
            break;
        case JE:
        case JNE:
        case JL:
        case JG:
        case JLE:
        case JGE:
        case LOOP:
            if (count != 1) {
                set_error(ctx, ASM_ERROR_INVALID_SYNTAX, line_number, operands[0],
                          "%s requires 1 operand", op_table[op].name);
                return false;
            }
            if (operands[0][0] == '%') {
                if (parse_register(operands[0]) < 0) {
                    set_error(ctx, ASM_ERROR_INVALID_REGISTER, line_number, operands[0],
                              "Invalid register '%s'", operands[0]);
                    return false;
                }
                *src1 = (uint8_t) parse_register(operands[0]);
                *src2 = *src1;
            } else {
                result = resolve_operand(ctx, operands[0], imm, &reg, &is_imm, true, resolve_symbols, line_number);
                if (!result) {
                    return false;
                }
                *mode = ADDR_MODE_IMM;
            }
            break;
        case CMP:
            if (count != 2) {
                set_error(ctx, ASM_ERROR_INVALID_SYNTAX, line_number, operands[0],
                          "CMP requires 2 operands");
                return false;
            }
            if (operands[0][0] == '$' || (operands[0][0] != '%' && operands[0][0] != '\0')) {
                result = resolve_operand(ctx, operands[0], imm, &reg, &is_imm, true, resolve_symbols, line_number);
                if (!result) {
                    return false;
                }
                if (!is_imm) {
                    set_error(ctx, ASM_ERROR_INVALID_SYNTAX, line_number, operands[0],
                              "CMP first operand must be immediate or register");
                    return false;
                }
                *mode = ADDR_MODE_IMM;
                if (parse_register(operands[1]) < 0) {
                    set_error(ctx, ASM_ERROR_INVALID_REGISTER, line_number, operands[1],
                              "Invalid register '%s'", operands[1]);
                    return false;
                }
                *src1 = (uint8_t) parse_register(operands[1]);
            } else {
                if (parse_register(operands[0]) < 0) {
                    set_error(ctx, ASM_ERROR_INVALID_REGISTER, line_number, operands[0],
                              "Invalid register '%s'", operands[0]);
                    return false;
                }
                *src1 = (uint8_t) parse_register(operands[0]);
                if (operands[1][0] == '$' || (operands[1][0] != '%' && operands[1][0] != '\0')) {
                    result = resolve_operand(ctx, operands[1], imm, &reg, &is_imm, true, resolve_symbols, line_number);
                    if (!result) {
                        return false;
                    }
                    if (!is_imm) {
                        set_error(ctx, ASM_ERROR_INVALID_SYNTAX, line_number, operands[1],
                                  "CMP second operand must be immediate, symbol, or register");
                        return false;
                    }
                    *mode = ADDR_MODE_IMM;
                } else {
                    if (parse_register(operands[1]) < 0) {
                        set_error(ctx, ASM_ERROR_INVALID_REGISTER, line_number, operands[1],
                                  "Invalid register '%s'", operands[1]);
                        return false;
                    }
                    *src2 = (uint8_t) parse_register(operands[1]);
                }
            }
            break;
        case LOAD:
            if (count != 2) {
                set_error(ctx, ASM_ERROR_INVALID_SYNTAX, line_number, operands[0],
                          "LOAD requires 2 operands");
                return false;
            }
            if (operands[0][0] == '%') {
                if (parse_register(operands[0]) < 0) {
                    set_error(ctx, ASM_ERROR_INVALID_REGISTER, line_number, operands[0],
                              "Invalid register '%s'", operands[0]);
                    return false;
                }
                *src1 = (uint8_t) parse_register(operands[0]);
            } else {
                result = resolve_operand(ctx, operands[0], imm, &reg, &is_imm, true, resolve_symbols, line_number);
                if (!result) {
                    return false;
                }
                if (!is_imm) {
                    set_error(ctx, ASM_ERROR_INVALID_SYNTAX, line_number, operands[0],
                              "LOAD source must be immediate, symbol, or register");
                    return false;
                }
                *mode = ADDR_MODE_IMM;
            }
            if (parse_register(operands[1]) < 0) {
                set_error(ctx, ASM_ERROR_INVALID_REGISTER, line_number, operands[1],
                          "Invalid destination register '%s'", operands[1]);
                return false;
            }
            *dst = (uint8_t) parse_register(operands[1]);
            break;
        case STORE:
            if (count != 2) {
                set_error(ctx, ASM_ERROR_INVALID_SYNTAX, line_number, operands[0],
                          "STORE requires 2 operands");
                return false;
            }
            if (parse_register(operands[0]) < 0) {
                set_error(ctx, ASM_ERROR_INVALID_REGISTER, line_number, operands[0],
                          "Invalid register '%s'", operands[0]);
                return false;
            }
            *src1 = (uint8_t) parse_register(operands[0]);
            if (operands[1][0] == '%') {
                if (parse_register(operands[1]) < 0) {
                    set_error(ctx, ASM_ERROR_INVALID_REGISTER, line_number, operands[1],
                              "Invalid register '%s'", operands[1]);
                    return false;
                }
                *src2 = (uint8_t) parse_register(operands[1]);
            } else {
                result = resolve_operand(ctx, operands[1], imm, &reg, &is_imm, true, resolve_symbols, line_number);
                if (!result) {
                    return false;
                }
                if (!is_imm) {
                    set_error(ctx, ASM_ERROR_INVALID_SYNTAX, line_number, operands[1],
                              "STORE target must be immediate, symbol, or register");
                    return false;
                }
                *mode = ADDR_MODE_IMM;
            }
            *dst = *src1;
            break;
        case PUSH:
            if (count != 1) {
                set_error(ctx, ASM_ERROR_INVALID_SYNTAX, line_number, operands[0],
                          "PUSH requires 1 operand");
                return false;
            }
            if (operands[0][0] == '%') {
                if (parse_register(operands[0]) < 0) {
                    set_error(ctx, ASM_ERROR_INVALID_REGISTER, line_number, operands[0],
                              "Invalid register '%s'", operands[0]);
                    return false;
                }
                *src1 = (uint8_t) parse_register(operands[0]);
            } else {
                result = resolve_operand(ctx, operands[0], imm, &reg, &is_imm, true, resolve_symbols, line_number);
                if (!result) {
                    return false;
                }
                if (!is_imm) {
                    set_error(ctx, ASM_ERROR_INVALID_SYNTAX, line_number, operands[0],
                              "PUSH operand must be immediate, symbol, or register");
                    return false;
                }
                *mode = ADDR_MODE_IMM;
            }
            break;
        case POP:
        case SETZ:
        case TIME:
        case DEBUG:
            if (count != 1) {
                set_error(ctx, ASM_ERROR_INVALID_SYNTAX, line_number, operands[0],
                          "%s requires 1 operand", op_table[op].name);
                return false;
            }
            if (parse_register(operands[0]) < 0) {
                set_error(ctx, ASM_ERROR_INVALID_REGISTER, line_number, operands[0],
                          "Invalid register '%s'", operands[0]);
                return false;
            }
            *dst = (uint8_t) parse_register(operands[0]);
            break;
        case RET:
        case EOP:
            if (count != 0) {
                set_error(ctx, ASM_ERROR_INVALID_SYNTAX, line_number, operands[0],
                          "%s takes no operands", op_table[op].name);
                return false;
            }
            break;
        case EOPV:
            if (count != 1) {
                set_error(ctx, ASM_ERROR_INVALID_SYNTAX, line_number, operands[0],
                          "EOPV requires 1 operand");
                return false;
            }
            if (operands[0][0] == '%') {
                if (parse_register(operands[0]) < 0) {
                    set_error(ctx, ASM_ERROR_INVALID_REGISTER, line_number, operands[0],
                              "Invalid register '%s'", operands[0]);
                    return false;
                }
                *src1 = (uint8_t) parse_register(operands[0]);
            } else {
                result = resolve_operand(ctx, operands[0], imm, &reg, &is_imm, true, resolve_symbols, line_number);
                if (!result) {
                    return false;
                }
                if (!is_imm) {
                    set_error(ctx, ASM_ERROR_INVALID_SYNTAX, line_number, operands[0],
                              "EOPV operand must be immediate, symbol, or register");
                    return false;
                }
                *mode = ADDR_MODE_IMM;
            }
            break;
        case SYSCALL:
            if (count != 1) {
                set_error(ctx, ASM_ERROR_INVALID_SYNTAX, line_number, operands[0],
                          "SYSCALL requires 1 operand");
                return false;
            }
            if (operands[0][0] == '%') {
                if (parse_register(operands[0]) < 0) {
                    set_error(ctx, ASM_ERROR_INVALID_REGISTER, line_number, operands[0],
                              "Invalid register '%s'", operands[0]);
                    return false;
                }
                *src1 = (uint8_t) parse_register(operands[0]);
            } else {
                result = resolve_operand(ctx, operands[0], imm, &reg, &is_imm, true, resolve_symbols, line_number);
                if (!result) {
                    return false;
                }
                if (!is_imm) {
                    set_error(ctx, ASM_ERROR_INVALID_SYNTAX, line_number, operands[0],
                              "SYSCALL operand must be immediate, symbol, or register");
                    return false;
                }
                *mode = ADDR_MODE_IMM;
            }
            break;
        default:
            set_error(ctx, ASM_ERROR_INVALID_OPCODE, line_number, operands[0],
                      "Invalid opcode '%s'", op_table[op].name);
            return false;
    }

    if (*mode & ADDR_MODE_IMM) {
        if (*imm > INT32_MAX || *imm < INT32_MIN) {
            *mode |= ADDR_MODE_IMM8;
        }
    }
    return true;
}

/**
 * @brief Parses a single line of assembly code.
 *
 * This function parses a line of assembly code, extracting the instruction,
 * operands, and any label. It handles comments and whitespace trimming.
 *
 * @param line The line of code to parse (modified in-place).
 * @param instr Buffer to store the instruction name.
 * @param operands Array to store the parsed operands.
 * @param operand_count Pointer to store the number of operands found.
 * @param label Buffer to store any label found.
 * @param ctx Pointer to the assembler context for error reporting.
 * @param line_number The line number for error reporting.
 * @return true if parsing succeeds, false otherwise.
 */
static bool
parse_line(char *line,
           char *instr,
           char operands[3][64],
           int *operand_count,
           char *label,
           AsmContext *ctx,
           int line_number) {
    *operand_count = 0;
    label[0] = '\0';

    char *comment_start = strchr(line, '#');
    if (comment_start) {
        *comment_start = '\0';
    }

    char *trimmed = trim(line);

    if (*trimmed == '\0') {
        return false;
    }

    char *colon = strchr(trimmed, ':');
    if (colon) {
        *colon = '\0';
        strncpy(label, trim(trimmed), 63);
        label[63] = '\0';
        trimmed = trim(colon + 1);
        if (*trimmed == '\0') {
            return true;
        }
    }

    char *token = strtok(trimmed, "\t ");

    if (!token) {
        return false;
    }

    strncpy(instr, token, 31);
    instr[31] = '\0';

    char *operand_text = strtok(NULL, "");
    if (operand_text) {
        if (!split_operands(operand_text, operands, operand_count, ctx, line_number)) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Performs the first pass of assembly, collecting symbols and calculating size.
 *
 * This function scans the source code to identify labels and calculate the total
 * bytecode size without resolving symbol references.
 *
 * @param ctx Pointer to the assembler context.
 * @param source The assembly source code.
 * @param size_out Pointer to store the calculated bytecode size.
 * @return true if the first pass succeeds, false otherwise.
 */
static bool
first_pass(AsmContext *ctx,
           const char *source,
           size_t *size_out) {
    symbol_count = 0;
    size_t pos = 0;
    char *src_copy = strdup(source);

    if (!src_copy) {
        return false;
    }

    char *saveptr;
    int line_number = 0;
    char *line = strtok_r(src_copy, "\n", &saveptr);
    while (line != NULL) {
        line_number++;
        char *line_to_trim = strdup(line);
        if (!line_to_trim) {
            break;
        }
        char instr[32] = {0};
        char operands[3][64] = {{0}};
        int operand_count = 0;
        char label[64] = {0};

        if (!parse_line(line_to_trim, instr, operands, &operand_count, label, ctx, line_number)) {
            if (label[0] != '\0' && instr[0] == '\0') {
                free(line_to_trim);
                line = strtok_r(NULL, "\n", &saveptr);
                continue;
            }
            if (ctx->error_count) {
                free(line_to_trim);
                free(src_copy);
                return false;
            }
            free(line_to_trim);
            line = strtok_r(NULL, "\n", &saveptr);
            continue;
        }

        if (label[0] != '\0') {
            if (!add_symbol(ctx, label, pos, line_number)) {
                free(line_to_trim);
                free(src_copy);
                return false;
            }
        }

        if (instr[0] == '\0') {
            free(line_to_trim);
            line = strtok_r(NULL, "\n", &saveptr);
            continue;
        }

        int op = get_opcode(instr);
        if (op < 0) {
            set_error(ctx, ASM_ERROR_INVALID_OPCODE, line_number, instr, "Invalid opcode '%s'", instr);
            free(line_to_trim);
            free(src_copy);
            return false;
        }

        uint8_t mode = 0, dst = 0, src1 = 0, src2 = 0;
        int64_t imm = 0;
        if (!build_instruction_layout(ctx, (opcode_t) op, operands, operand_count,
                                      &mode, &dst, &src1, &src2, &imm, false, line_number)) {
            free(line_to_trim);
            free(src_copy);
            return false;
        }

        pos += instruction_size(mode);
        free(line_to_trim);
        line = strtok_r(NULL, "\n", &saveptr);
    }

    free(src_copy);
    *size_out = pos;
    return true;
}

/**
 * @brief Performs the second pass of assembly, generating bytecode.
 *
 * This function processes the source code again, resolving symbol references
 * and generating the final bytecode output.
 *
 * @param ctx Pointer to the assembler context.
 * @param source The assembly source code.
 * @param output Buffer to store the generated bytecode.
 * @param max_size Maximum size of the output buffer.
 * @param final_size Pointer to store the actual size of generated bytecode.
 * @return true if the second pass succeeds, false otherwise.
 */
static bool
second_pass(AsmContext *ctx,
            const char *source,
            uint8_t *output,
            size_t max_size,
            size_t *final_size) {
    size_t pos = 0;
    char *src_copy = strdup(source);

    if (!src_copy) {
        return false;
    }

    char *saveptr;
    int line_number = 0;
    char *line = strtok_r(src_copy, "\n", &saveptr);

    while (line != NULL) {
        line_number++;
        char *line_to_trim = strdup(line);
        if (!line_to_trim) {
            break;
        }
        char instr[32] = {0};
        char operands[3][64] = {{0}};
        int operand_count = 0;
        char label[64] = {0};

        if (!parse_line(line_to_trim, instr, operands, &operand_count, label, ctx, line_number)) {
            if (label[0] != '\0' && instr[0] == '\0') {
                free(line_to_trim);
                line = strtok_r(NULL, "\n", &saveptr);
                continue;
            }
            if (ctx->error_count) {
                free(line_to_trim);
                free(src_copy);
                return false;
            }
            free(line_to_trim);
            line = strtok_r(NULL, "\n", &saveptr);
            continue;
        }

        if (instr[0] == '\0') {
            free(line_to_trim);
            line = strtok_r(NULL, "\n", &saveptr);
            continue;
        }

        int op = get_opcode(instr);
        if (op < 0) {
            set_error(ctx, ASM_ERROR_INVALID_OPCODE, line_number, instr, "Invalid opcode '%s'", instr);
            free(line_to_trim);
            free(src_copy);
            return false;
        }

        uint8_t mode = 0, dst = 0, src1 = 0, src2 = 0;
        int64_t imm = 0;
        if (!build_instruction_layout(ctx, (opcode_t) op, operands, operand_count,
                                      &mode, &dst, &src1, &src2, &imm, true, line_number)) {
            free(line_to_trim);
            free(src_copy);
            return false;
        }

        size_t required = instruction_size(mode);
        if (output && pos + required > max_size) {
            set_error(ctx, ASM_ERROR_BUFFER_OVERFLOW, line_number, instr,
                      "Output buffer overflow while encoding '%s'", instr);
            free(line_to_trim);
            free(src_copy);
            return false;
        }

        if (!encode_instruction((uint8_t) op, mode, dst, src1, src2, imm, output, &pos, max_size)) {
            set_error(ctx, ASM_ERROR_BUFFER_OVERFLOW, line_number, instr,
                      "Output buffer overflow while encoding '%s'", instr);
            free(line_to_trim);
            free(src_copy);
            return false;
        }

        free(line_to_trim);
        line = strtok_r(NULL, "\n", &saveptr);
    }

    free(src_copy);
    *final_size = pos;
    return true;
}

/**
 * @brief Assembles assembly source code into bytecode.
 *
 * This function performs a two-pass assembly process: first collecting symbols
 * and calculating size, then generating the final bytecode with resolved references.
 *
 * @param ctx Pointer to the assembler context.
 * @param source The assembly source code to assemble.
 * @param output Buffer to store the generated bytecode.
 * @param max_size Maximum size of the output buffer.
 * @return The size of the generated bytecode, or 0 on error.
 */
size_t
asm_assemble(AsmContext *ctx,
             const char *source,
             uint8_t *output,
             size_t max_size) {
    if (!(ctx && source)) {
        return 0;
    }

    ctx->error_count = 0;
    ctx->total_lines = 0;
    symbol_count = 0;

    char *copy = strdup(source);
    if (!copy) {
        set_error(ctx, ASM_ERROR_INVALID_SYNTAX, 0, NULL, "Out of memory");
        return 0;
    }
    char *saveptr;
    char *line = strtok_r(copy, "\n", &saveptr);
    while (line != NULL) {
        ctx->total_lines++;
        line = strtok_r(NULL, "\n", &saveptr);
    }
    free(copy);

    size_t estimated_size = 0;
    if (!first_pass(ctx, source, &estimated_size)) {
        return 0;
    }

    size_t final_size = 0;
    if (!second_pass(ctx, source, output, max_size, &final_size)) {
        return 0;
    }

    return final_size;
}

/**
 * @brief Assembles assembly source code into bytecode using a temporary context.
 *
 * This is a convenience function that creates a temporary assembler context,
 * assembles the source code, and then frees the context.
 *
 * @param source The assembly source code to assemble.
 * @param output Buffer to store the generated bytecode.
 * @param max_size Maximum size of the output buffer.
 * @return The size of the generated bytecode, or 0 on error.
 */
size_t
assemble(const char *source,
         uint8_t *output,
         size_t max_size) {
    AsmContext *ctx = asm_create_context(false);

    if (!ctx) {
        return 0;
    }

    size_t result = asm_assemble(ctx, source, output, max_size);
    asm_free_context(ctx);

    return result;
}
