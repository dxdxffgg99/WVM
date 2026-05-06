#include "assembler.h"
#include "../bytecode/opcode.h"
#include "../cpu/cpu.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

static const OpMapping op_table[] = {
    {"add", ADD},
    {"and", AND},
    {"call", CALL},
    {"cmp", CMP},
    {"debug", DEBUG},
    {"dec", DEC},
    {"div", DIV},
    {"eop", EOP},
    {"eopv", EOPV},
    {"inc", INC},
    {"je", JE},
    {"jge", JGE},
    {"jg", JG},
    {"jl", JL},
    {"jle", JLE},
    {"jmp", JMP},
    {"jne", JNE},
    {"load", LOAD},
    {"loop", LOOP},
    {"lsh", LSH},
    {"mov", MOV},
    {"mul", MUL},
    {"nop", NOP},
    {"or", OR},
    {"pop", POP},
    {"push", PUSH},
    {"ret", RET},
    {"rsh", RSH},
    {"setz", SETZ},
    {"store", STORE},
    {"sub", SUB},
    {"syscall", SYSCALL},
    {"time", TIME},
    {"xor", XOR},
    {NULL, NOP}
};

typedef struct {
    char name[64];
    uint64_t addr;
} Symbol;

#define MAX_SYMBOLS 1024
static Symbol symbol_table[MAX_SYMBOLS];
static int symbol_count = 0;

static void add_symbol(const char *name, uint64_t addr) {
    if (symbol_count < MAX_SYMBOLS) {
        strncpy(symbol_table[symbol_count].name, name, 63);
        symbol_table[symbol_count].name[63] = '\0';
        symbol_table[symbol_count].addr = addr;
        symbol_count++;
    }
}

static int64_t find_symbol(const char *name) {
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) {
            return (int64_t) symbol_table[i].addr;
        }
    }
    return -1;
}

static int get_opcode(const char *name) {
    for (int i = 0; op_table[i].name != NULL; i++) {
        if (strcmp(name, op_table[i].name) == 0) return op_table[i].op;
    }
    return -1;
}

static int parse_register(const char *str) {
    if (str[0] != '%' || str[1] != 'r' || !isdigit((unsigned char)str[2])) return -1;
    char *endptr;
    long reg = strtol(&str[2], &endptr, 10);
    if (*endptr != '\0' || reg < 0 || reg > 255) return -1;
    return (int) reg;
}

static int64_t parse_immediate(const char *str, bool *is_imm) {
    *is_imm = false;
    if (str[0] != '$') return 0;
    errno = 0;
    char *endptr;
    int64_t imm = strtoll(&str[1], &endptr, 0);
    if (errno != 0 || *endptr != '\0') return 0;
    *is_imm = true;
    return imm;
}

static void encode_instruction(uint8_t opcode, uint8_t mode, uint8_t dst, uint8_t src1, uint8_t src2, int64_t imm,
                               uint8_t *out, size_t *pos) {
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
        *pos += 5;
        if (mode & ADDR_MODE_IMM) {
            *pos += (mode & ADDR_MODE_IMM8) ? 8 : 4;
        }
    }
}

static char *trim(char *str) {
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

size_t assemble(const char *source, uint8_t *output, size_t max_size) {
    symbol_count = 0;
    char *src_copy = strdup(source);
    if (!src_copy) {
        fprintf(stderr, "Error: Memory allocation failed for source copy\n");
        return 0;
    }
    size_t pos = 0;

    char *saveptr;
    char *line = strtok_r(src_copy, "\n", &saveptr);
    while (line != NULL) {
        char *line_to_trim = strdup(line);
        if (!line_to_trim) {
            fprintf(stderr, "Error: Memory allocation failed for line copy\n");
            free(src_copy);
            return 0;
        }
        char *comment_start = strchr(line_to_trim, '#');
        if (comment_start) *comment_start = '\0';
        char *trimmed = trim(line_to_trim);
        if (*trimmed == '\0') {
            free(line_to_trim);
            line = strtok_r(NULL, "\n", &saveptr);
            continue;
        }

        char *colon = strchr(trimmed, ':');
        if (colon) {
            *colon = '\0';
            char *label = trim(trimmed);
            if (symbol_count >= MAX_SYMBOLS) {
                fprintf(stderr, "Error: Symbol table overflow, maximum %d symbols allowed\n", MAX_SYMBOLS);
                free(line_to_trim);
                free(src_copy);
                return 0;
            }
            add_symbol(label, pos);
            trimmed = trim(colon + 1);
            if (*trimmed == '\0') {
                free(line_to_trim);
                line = strtok_r(NULL, "\n", &saveptr);
                continue;
            }
        }

        char instr[32], op1[64], op2[64], op3[64];
        op1[0] = op2[0] = op3[0] = '\0';
        int count = sscanf(trimmed, "%31s %63[^,], %63[^,], %63s", instr, op1, op2, op3);
        if (count >= 1) {
            int op = get_opcode(instr);
            if (op == -1) {
                fprintf(stderr, "Error: Unknown opcode '%s'\n", instr);
                free(line_to_trim);
                free(src_copy);
                return 0;
            }
            uint8_t mode = 0;
            char *ops[3] = {trim(op1), trim(op2), trim(op3)};
            for (int i = 0; i < 3; i++) {
                if (ops[i][0] == '$') {
                    bool is_imm;
                    int64_t imm = parse_immediate(ops[i], &is_imm);
                    if (!is_imm) {
                        fprintf(stderr, "Error: Invalid immediate value '%s'\n", ops[i]);
                        free(line_to_trim);
                        free(src_copy);
                        return 0;
                    }
                    mode |= ADDR_MODE_IMM;
                    if (imm > 0xFFFFFFFFLL || imm < -2147483648LL)
                        mode |= ADDR_MODE_IMM8;
                    break;
                } else if (ops[i][0] != '\0' && ops[i][0] != '%' &&
                           op != NOP && op != RET && op != EOP) {
                    mode |= ADDR_MODE_IMM;
                    break;
                }
            }
            encode_instruction((uint8_t) op, mode, 0, 0, 0, 0, NULL, &pos);
        }
        free(line_to_trim);
        line = strtok_r(NULL, "\n", &saveptr);
    }
    free(src_copy);

    src_copy = strdup(source);
    if (!src_copy) {
        fprintf(stderr, "Error: Memory allocation failed for second source copy\n");
        return 0;
    }
    size_t final_pos = 0;
    line = strtok_r(src_copy, "\n", &saveptr);
    while (line != NULL) {
        char *line_to_trim = strdup(line);
        if (!line_to_trim) {
            fprintf(stderr, "Error: Memory allocation failed for line copy in second pass\n");
            free(src_copy);
            return 0;
        }
        char *comment_start = strchr(line_to_trim, '#');
        if (comment_start) *comment_start = '\0';
        char *trimmed = trim(line_to_trim);
        if (*trimmed == '\0') {
            free(line_to_trim);
            line = strtok_r(NULL, "\n", &saveptr);
            continue;
        }

        char *colon = strchr(trimmed, ':');
        if (colon) {
            trimmed = trim(colon + 1);
            if (*trimmed == '\0') {
                free(line_to_trim);
                line = strtok_r(NULL, "\n", &saveptr);
                continue;
            }
        }

        char instr[32], op1[64], op2[64], op3[64];
        op1[0] = op2[0] = op3[0] = '\0';
        int count = sscanf(trimmed, "%31s %63[^,], %63[^,], %63s", instr, op1, op2, op3);

        int op = get_opcode(instr);
        if (op != -1) {
            uint8_t mode = 0, dst = 0, src1 = 0, src2 = 0;
            int64_t imm = 0;
            bool is_imm = false;

            char *t_op1 = trim(op1);
            char *t_op2 = trim(op2);
            char *t_op3 = trim(op3);

            switch (op) {
                case MOV:
                    if (t_op1[0] == '$') {
                        imm = parse_immediate(t_op1, &is_imm);
                        if (!is_imm) {
                            fprintf(stderr, "Error: Invalid immediate value '%s' in MOV\n", t_op1);
                            free(line_to_trim);
                            free(src_copy);
                            return 0;
                        }
                        mode = ADDR_MODE_IMM;
                        if (imm > 0xFFFFFFFFLL || imm < -2147483648LL) mode |= ADDR_MODE_IMM8;
                        int reg = parse_register(t_op2);
                        if (reg == -1) {
                            fprintf(stderr, "Error: Invalid register '%s' in MOV\n", t_op2);
                            free(line_to_trim);
                            free(src_copy);
                            return 0;
                        }
                        dst = (uint8_t) reg;
                    } else {
                        int reg1 = parse_register(t_op1);
                        int reg2 = parse_register(t_op2);
                        if (reg1 == -1 || reg2 == -1) {
                            fprintf(stderr, "Error: Invalid register(s) '%s', '%s' in MOV\n", t_op1, t_op2);
                            free(line_to_trim);
                            free(src_copy);
                            return 0;
                        }
                        src1 = (uint8_t) reg1;
                        dst = (uint8_t) reg2;
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
                    if (t_op1[0] == '$') {
                        imm = parse_immediate(t_op1, &is_imm);
                        if (!is_imm) {
                            fprintf(stderr, "Error: Invalid immediate value '%s'\n", t_op1);
                            free(line_to_trim);
                            free(src_copy);
                            return 0;
                        }
                        mode = ADDR_MODE_IMM;
                        if (imm > 0xFFFFFFFFLL || imm < -2147483648LL) mode |= ADDR_MODE_IMM8;
                        int reg = parse_register(t_op2);
                        if (reg == -1) {
                            fprintf(stderr, "Error: Invalid register '%s' in MOV\n", t_op2);
                            free(line_to_trim);
                            free(src_copy);
                            return 0;
                        }
                        dst = (uint8_t) reg;
                        src1 = dst;
                    } else if (count == 3) {
                        int reg1 = parse_register(t_op1);
                        int reg2 = parse_register(t_op2);
                        if (reg1 == -1 || reg2 == -1) {
                            fprintf(stderr, "Error: Invalid register(s) '%s', '%s'\n", t_op1, t_op2);
                            free(line_to_trim);
                            free(src_copy);
                            return 0;
                        }
                        src2 = (uint8_t) reg1;
                        dst = (uint8_t) reg2;
                        src1 = dst;
                    } else if (count == 4) {
                        int reg1 = parse_register(t_op1);
                        if (reg1 == -1) {
                            fprintf(stderr, "Error: Invalid register '%s'\n", t_op1);
                            free(line_to_trim);
                            free(src_copy);
                            return 0;
                        }
                        src1 = (uint8_t) reg1;
                        if (t_op2[0] == '$') {
                            imm = parse_immediate(t_op2, &is_imm);
                            if (!is_imm) {
                                fprintf(stderr, "Error: Invalid immediate value '%s'\n", t_op2);
                                free(line_to_trim);
                                free(src_copy);
                                return 0;
                            }
                            mode = ADDR_MODE_IMM;
                            if (imm > 0xFFFFFFFFLL || imm < -2147483648LL)
                                mode |= ADDR_MODE_IMM8;
                        } else {
                            int reg2 = parse_register(t_op2);
                            if (reg2 == -1) {
                                fprintf(stderr, "Error: Invalid register '%s'\n", t_op2);
                                free(line_to_trim);
                                free(src_copy);
                                return 0;
                            }
                            src2 = (uint8_t) reg2;
                        }
                        int reg3 = parse_register(t_op3);
                        if (reg3 == -1) {
                            fprintf(stderr, "Error: Invalid register '%s'\n", t_op3);
                            free(line_to_trim);
                            free(src_copy);
                            return 0;
                        }
                        dst = (uint8_t) reg3;
                    }
                    break;
                case INC:
                case DEC:
                    int reg_inc = parse_register(t_op1);
                    if (reg_inc == -1) {
                        fprintf(stderr, "Error: Invalid register '%s' in INC/DEC\n", t_op1);
                        free(line_to_trim);
                        free(src_copy);
                        return 0;
                    }
                    dst = (uint8_t) reg_inc;
                    break;
                case JMP:
                case CALL:
                    if (t_op1[0] == '$') {
                        imm = parse_immediate(t_op1, &is_imm);
                        if (!is_imm) {
                            fprintf(stderr, "Error: Invalid immediate value '%s' in JMP/CALL\n", t_op1);
                            free(line_to_trim);
                            free(src_copy);
                            return 0;
                        }
                        mode = ADDR_MODE_IMM;
                    } else if (t_op1[0] == '%') {
                        int reg = parse_register(t_op1);
                        if (reg == -1) {
                            fprintf(stderr, "Error: Invalid register '%s' in JMP/CALL\n", t_op1);
                            free(line_to_trim);
                            free(src_copy);
                            return 0;
                        }
                        src1 = (uint8_t) reg;
                        src2 = src1;
                    } else {
                        imm = find_symbol(t_op1);
                        if (imm == -1) {
                            fprintf(stderr, "Error: Undefined symbol '%s' in JMP/CALL\n", t_op1);
                            free(line_to_trim);
                            free(src_copy);
                            return 0;
                        }
                        mode = ADDR_MODE_IMM;
                    }
                    break;
                case JE:
                case JNE:
                case JL:
                case JG:
                case JLE:
                case JGE:
                    if (t_op1[0] == '$') {
                        imm = parse_immediate(t_op1, &is_imm);
                        if (!is_imm) {
                            fprintf(stderr, "Error: Invalid immediate value '%s' in jump\n", t_op1);
                            free(line_to_trim);
                            free(src_copy);
                            return 0;
                        }
                        mode = ADDR_MODE_IMM;
                    } else {
                        imm = find_symbol(t_op1);
                        if (imm == -1) {
                            fprintf(stderr, "Error: Undefined symbol '%s' in jump\n", t_op1);
                            free(line_to_trim);
                            free(src_copy);
                            return 0;
                        }
                        mode = ADDR_MODE_IMM;
                    }
                    break;
                case LOOP:
                    if (t_op1[0] == '$') {
                        imm = parse_immediate(t_op1, &is_imm);
                        if (!is_imm) {
                            fprintf(stderr, "Error: Invalid immediate value '%s' in jump\n", t_op1);
                            free(line_to_trim);
                            free(src_copy);
                            return 0;
                        }
                        mode = ADDR_MODE_IMM;
                    } else {
                        imm = find_symbol(t_op1);
                        if (imm == -1) {
                            fprintf(stderr, "Error: Undefined symbol '%s' in jump\n", t_op1);
                            free(line_to_trim);
                            free(src_copy);
                            return 0;
                        }
                        mode = ADDR_MODE_IMM;
                    }
                    int reg_jump = parse_register(t_op2);
                    if (reg_jump == -1) {
                        fprintf(stderr, "Error: Invalid register '%s' in jump\n", t_op2);
                        free(line_to_trim);
                        free(src_copy);
                        return 0;
                    }
                    dst = (uint8_t) reg_jump;
                    src1 = dst;
                    break;
                case CMP:
                    if (t_op1[0] == '$') {
                        imm = parse_immediate(t_op1, &is_imm);
                        if (!is_imm) {
                            fprintf(stderr, "Error: Invalid immediate value '%s' in CMP\n", t_op1);
                            free(line_to_trim);
                            free(src_copy);
                            return 0;
                        }
                        mode = ADDR_MODE_IMM;
                        if (imm > 0xFFFFFFFFLL || imm < -2147483648LL) mode |= ADDR_MODE_IMM8;
                        int reg = parse_register(t_op2);
                        if (reg == -1) {
                            fprintf(stderr, "Error: Invalid register '%s' in CMP\n", t_op2);
                            free(line_to_trim);
                            free(src_copy);
                            return 0;
                        }
                        src1 = (uint8_t) reg;
                    } else {
                        int reg1 = parse_register(t_op1);
                        if (reg1 == -1) {
                            fprintf(stderr, "Error: Invalid register '%s' in CMP\n", t_op1);
                            free(line_to_trim);
                            free(src_copy);
                            return 0;
                        }
                        src1 = (uint8_t) reg1;
                        if (t_op2[0] == '$') {
                            imm = parse_immediate(t_op2, &is_imm);
                            if (!is_imm) {
                                fprintf(stderr, "Error: Invalid immediate value '%s' in CMP\n", t_op2);
                                free(line_to_trim);
                                free(src_copy);
                                return 0;
                            }
                            mode = ADDR_MODE_IMM;
                            if (imm > 0xFFFFFFFFLL || imm < -2147483648LL)
                                mode |= ADDR_MODE_IMM8;
                        } else {
                            int reg2 = parse_register(t_op2);
                            if (reg2 == -1) {
                                fprintf(stderr, "Error: Invalid register '%s' in CMP\n", t_op2);
                                free(line_to_trim);
                                free(src_copy);
                                return 0;
                            }
                            src2 = (uint8_t) reg2;
                        }
                    }
                    break;
                case LOAD:
                    if (t_op1[0] == '$') {
                        imm = parse_immediate(t_op1, &is_imm);
                        if (!is_imm) {
                            fprintf(stderr, "Error: Invalid immediate value '%s' in LOAD\n", t_op1);
                            free(line_to_trim);
                            free(src_copy);
                            return 0;
                        }
                        mode = ADDR_MODE_IMM;
                    } else {
                        int reg = parse_register(t_op1);
                        if (reg == -1) {
                            fprintf(stderr, "Error: Invalid register '%s' in LOAD\n", t_op1);
                            free(line_to_trim);
                            free(src_copy);
                            return 0;
                        }
                        src1 = (uint8_t) reg;
                    }
                    int reg_load_dst = parse_register(t_op2);
                    if (reg_load_dst == -1) {
                        fprintf(stderr, "Error: Invalid register '%s' in LOAD\n", t_op2);
                        free(line_to_trim);
                        free(src_copy);
                        return 0;
                    }
                    dst = (uint8_t) reg_load_dst;
                    break;
                case STORE:
                    int reg_store_src = parse_register(t_op1);
                    if (reg_store_src == -1) {
                        fprintf(stderr, "Error: Invalid register '%s' in STORE\n", t_op1);
                        free(line_to_trim);
                        free(src_copy);
                        return 0;
                    }
                    src1 = (uint8_t) reg_store_src;
                    if (t_op2[0] == '$') {
                        imm = parse_immediate(t_op2, &is_imm);
                        if (!is_imm) {
                            fprintf(stderr, "Error: Invalid immediate value '%s' in STORE\n", t_op2);
                            free(line_to_trim);
                            free(src_copy);
                            return 0;
                        }
                        mode = ADDR_MODE_IMM;
                    } else {
                        int reg2 = parse_register(t_op2);
                        if (reg2 == -1) {
                            fprintf(stderr, "Error: Invalid register '%s' in STORE\n", t_op2);
                            free(line_to_trim);
                            free(src_copy);
                            return 0;
                        }
                        src2 = (uint8_t) reg2;
                    }
                    dst = src1;
                    break;
                case PUSH:
                    if (t_op1[0] == '$') {
                        imm = parse_immediate(t_op1, &is_imm);
                        if (!is_imm) {
                            fprintf(stderr, "Error: Invalid immediate value '%s' in PUSH\n", t_op1);
                            free(line_to_trim);
                            free(src_copy);
                            return 0;
                        }
                        mode = ADDR_MODE_IMM;
                    } else {
                        int reg_push = parse_register(t_op1);
                        if (reg_push == -1) {
                            fprintf(stderr, "Error: Invalid register '%s' in PUSH\n", t_op1);
                            free(line_to_trim);
                            free(src_copy);
                            return 0;
                        }
                        src1 = (uint8_t) reg_push;
                    }
                    break;
                case POP:
                    int reg_pop_dst = parse_register(t_op1);
                    if (reg_pop_dst == -1) {
                        fprintf(stderr, "Error: Invalid register '%s' in POP\n", t_op1);
                        free(line_to_trim);
                        free(src_copy);
                        return 0;
                    }
                    dst = (uint8_t) reg_pop_dst;
                    break;
                case SETZ:
                case TIME:
                case DEBUG:
                    int reg_set_dst = parse_register(t_op1);
                    if (reg_set_dst == -1) {
                        fprintf(stderr, "Error: Invalid register '%s'\n", t_op1);
                        free(line_to_trim);
                        free(src_copy);
                        return 0;
                    }
                    dst = (uint8_t) reg_set_dst;
                    break;
                case RET:
                case EOP:
                    break;
                case EOPV:
                    if (t_op1[0] == '$') {
                        imm = parse_immediate(t_op1, &is_imm);
                        if (!is_imm) {
                            fprintf(stderr, "Error: Invalid immediate value '%s' in EOPV\n", t_op1);
                            free(line_to_trim);
                            free(src_copy);
                            return 0;
                        }
                        mode = ADDR_MODE_IMM;
                    } else {
                        int reg_eopv = parse_register(t_op1);
                        if (reg_eopv == -1) {
                            fprintf(stderr, "Error: Invalid register '%s' in EOPV\n", t_op1);
                            free(line_to_trim);
                            free(src_copy);
                            return 0;
                        }
                        src1 = (uint8_t) reg_eopv;
                    }
                    break;
                case SYSCALL:
                    if (t_op1[0] == '$') {
                        imm = parse_immediate(t_op1, &is_imm);
                        if (!is_imm) {
                            fprintf(stderr, "Error: Invalid immediate value '%s' in SYSCALL\n", t_op1);
                            free(line_to_trim);
                            free(src_copy);
                            return 0;
                        }
                        mode = ADDR_MODE_IMM;
                        if (imm > 0xFFFFFFFFLL || imm < -2147483648LL) mode |= ADDR_MODE_IMM8;
                    } else if (t_op1[0] == '%') {
                        int reg_syscall = parse_register(t_op1);
                        if (reg_syscall == -1) {
                            fprintf(stderr, "Error: Invalid register '%s' in SYSCALL\n", t_op1);
                            free(line_to_trim);
                            free(src_copy);
                            return 0;
                        }
                        src1 = (uint8_t) reg_syscall;
                    }
                    break;
                default: break;
            }

            size_t instr_size = 5;
            if (mode & ADDR_MODE_IMM) {
                instr_size += (mode & ADDR_MODE_IMM8) ? 8 : 4;
            }
            if (output && final_pos + instr_size > max_size) {
                fprintf(stderr, "Error: Output buffer overflow, required size %zu, max size %zu\n", final_pos + instr_size, max_size);
                free(line_to_trim);
                free(src_copy);
                return 0;
            }
            if (output && final_pos + instr_size <= max_size) {
                encode_instruction((uint8_t) op, mode, dst, src1, src2, imm, output, &final_pos);
            } else if (!output) {
                encode_instruction((uint8_t) op, mode, dst, src1, src2, imm, NULL, &final_pos);
            }
        }
        free(line_to_trim);
        line = strtok_r(NULL, "\n", &saveptr);
    }
    free(src_copy);
    return final_pos;
}
