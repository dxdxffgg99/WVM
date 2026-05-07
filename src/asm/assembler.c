#include "assembler.h"
#include "../bytecode/opcode.h"
#include "../cpu/cpu.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

static const OpMapping opTable[] = {
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
static Symbol symbolTable[MAX_SYMBOLS];
static int symbolCount = 0;

static char *trim(char *str) {
    while (isspace((unsigned char)*str)) str++;

    if (*str == 0) return str;

    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    end[1] = '\0';
    return str;
}

static void add_symbol(const char *name, uint64_t addr) {
    if (symbolCount < MAX_SYMBOLS) {
        strncpy(symbolTable[symbolCount].name, name, 63);
        symbolTable[symbolCount].name[63] = '\0';
        symbolTable[symbolCount].addr = addr;
        symbolCount++;
    }
}

static int64_t find_symbol(const char *name) {
    for (int i = 0; i < symbolCount; i++) {
        if (strcmp(symbolTable[i].name, name) == 0) {
            return (int64_t) symbolTable[i].addr;
        }
    }
    return -1;
}

static int get_opcode(const char *name) {
    for (int i = 0; opTable[i].name != NULL; i++) {
        if (strcmp(name, opTable[i].name) == 0) return opTable[i].op;
    }

    return -1;
}

static int parse_register(const char *str) {
    if (str[0] != '%' ||
        str[1] != 'r' ||
        !isdigit((unsigned char)str[2]))
    {
        return -1;
    }

    char *endptr;
    long reg = strtol(&str[2], &endptr, 10);
    if (*endptr != '\0' ||
        reg < 0 ||
        reg > REG_COUNT)
    {
        return -1;
    }

    return (int)reg;
}

static int64_t parse_immediate(const char *str, bool *isImm) {
    *isImm = false;

    if (str[0] != '$') return 0;

    errno = 0;
    char *endptr;
    int64_t imm = strtoll(&str[1], &endptr, 0);

    if (errno != 0 || *endptr != '\0') return 0;

    *isImm = true;
    return imm;
}

static char *preprocess_line(char *line, char **lineToFree) {
    *lineToFree = strdup(line);
    if (!*lineToFree) return NULL;
    char *comment_start = strchr(*lineToFree, '#');
    if (comment_start) *comment_start = '\0';
    return trim(*lineToFree);
}

static int check_reg(const char *name, const char *regStr, char *lineToFree, char *srcCopy) {
    int reg = parse_register(regStr);

    if (reg == -1) {
        fprintf(stderr, "Error: Invalid register '%s' in %s\n", regStr, name);
        free(lineToFree);
        free(srcCopy);
    }

    return reg;
}

static int64_t check_imm(const char *name, const char *immStr, bool *isImm, char *lineToFree, char *srcCopy) {
    int64_t imm = parse_immediate(immStr, isImm);
    if (!*isImm) {
        fprintf(stderr, "Error: Invalid immediate value '%s' in %s\n", immStr, name);
        free(lineToFree);
        free(srcCopy);
    }
    return imm;
}

static void
encode_instruction(uint8_t opcode,
                   uint8_t mode,
                   uint8_t dst,
                   uint8_t src1,
                   uint8_t src2,
                   int64_t imm,
                   uint8_t *out,
                   size_t *pos)
{
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

size_t assemble(const char *source, uint8_t *output, size_t maxSize) {
    symbolCount = 0;
    char *srcCopy = strdup(source);

    if (!srcCopy) {
        fprintf(stderr, "Error: Memory allocation failed for source copy\n");
        return 0;
    }

    size_t pos = 0;
    char *savePtr;
    char *line = strtok_r(srcCopy, "\n", &savePtr);

    while (line != NULL) {
        char *lineToTrim;
        char *trimmed = preprocess_line(line, &lineToTrim);

        if (!lineToTrim) {
            fprintf(stderr, "Error: Memory allocation failed for line copy\n");
            free(srcCopy);
            return 0;
        }

        if (*trimmed == '\0') {
            free(lineToTrim);
            line = strtok_r(NULL, "\n", &savePtr);
            continue;
        }

        char *colon = strchr(trimmed, ':');
        if (colon) {
            *colon = '\0';
            char *label = trim(trimmed);

            if (symbolCount >= MAX_SYMBOLS) {
                fprintf(stderr, "Error: Symbol table overflow, maximum %d symbols allowed\n", MAX_SYMBOLS);
                free(lineToTrim);
                free(srcCopy);
                return 0;
            }

            add_symbol(label, pos);
            trimmed = trim(colon + 1);

            if (*trimmed == '\0') {
                free(lineToTrim);
                line = strtok_r(NULL, "\n", &savePtr);
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
                free(lineToTrim);
                free(srcCopy);
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
                        free(lineToTrim);
                        free(srcCopy);
                        return 0;
                    }

                    mode |= ADDR_MODE_IMM;
                    if (imm > 0xFFFFFFFFLL || imm < -2147483648LL)
                        mode |= ADDR_MODE_IMM8;
                    break;
                } else if (ops[i][0] != '\0' &&
                           ops[i][0] != '%' &&
                           op != NOP &&
                           op != RET &&
                           op != EOP)
                {
                    mode |= ADDR_MODE_IMM;
                    break;
                }
            }

            encode_instruction((uint8_t) op, mode, 0, 0, 0, 0, NULL, &pos);
        }

        free(lineToTrim);
        line = strtok_r(NULL, "\n", &savePtr);
    }

    free(srcCopy);
    srcCopy = strdup(source);

    if (!srcCopy) {
        fprintf(stderr, "Error: Memory allocation failed for second source copy\n");
        return 0;
    }

    size_t finalPos = 0;
    line = strtok_r(srcCopy, "\n", &savePtr);

    while (line != NULL) {
        char *lineToTrim;
        char *trimmed = preprocess_line(line, &lineToTrim);

        if (!lineToTrim) {
            fprintf(stderr, "Error: Memory allocation failed for line copy in second pass\n");
            free(srcCopy);
            return 0;
        }

        if (*trimmed == '\0') {
            free(lineToTrim);
            line = strtok_r(NULL, "\n", &savePtr);
            continue;
        }

        char *colon = strchr(trimmed, ':');
        if (colon) {
            trimmed = trim(colon + 1);

            if (*trimmed == '\0') {
                free(lineToTrim);
                line = strtok_r(NULL, "\n", &savePtr);
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
            bool isImm = false;

            char *t_op1 = trim(op1);
            char *t_op2 = trim(op2);
            char *t_op3 = trim(op3);

            switch (op) {
                case MOV:
                    if (t_op1[0] == '$') {
                        imm = check_imm("MOV", t_op1, &isImm, lineToTrim, srcCopy);
                        if (!isImm) return 0;
                        mode = ADDR_MODE_IMM;
                        if (imm > 0xFFFFFFFFLL || imm < -2147483648LL) mode |= ADDR_MODE_IMM8;
                        int reg = check_reg("MOV", t_op2, lineToTrim, srcCopy);
                        if (reg == -1) return 0;
                        dst = (uint8_t) reg;
                    } else {
                        int reg1 = check_reg("MOV", t_op1, lineToTrim, srcCopy);
                        int reg2 = check_reg("MOV", t_op2, lineToTrim, srcCopy);
                        if (reg1 == -1 || reg2 == -1) return 0;
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
                        imm = check_imm(instr, t_op1, &isImm, lineToTrim, srcCopy);
                        if (!isImm) return 0;
                        mode = ADDR_MODE_IMM;
                        if (imm > 0xFFFFFFFFLL || imm < -2147483648LL) mode |= ADDR_MODE_IMM8;
                        int reg = check_reg(instr, t_op2, lineToTrim, srcCopy);
                        if (reg == -1) return 0;
                        dst = (uint8_t) reg;
                        src1 = dst;
                    } else if (count == 3) {
                        int reg1 = check_reg(instr, t_op1, lineToTrim, srcCopy);
                        int reg2 = check_reg(instr, t_op2, lineToTrim, srcCopy);
                        if (reg1 == -1 || reg2 == -1) return 0;
                        src2 = (uint8_t) reg1;
                        dst = (uint8_t) reg2;
                        src1 = dst;
                    } else if (count == 4) {
                        int reg1 = check_reg(instr, t_op1, lineToTrim, srcCopy);
                        if (reg1 == -1) return 0;
                        src1 = (uint8_t) reg1;
                        if (t_op2[0] == '$') {
                            imm = check_imm(instr, t_op2, &isImm, lineToTrim, srcCopy);
                            if (!isImm) return 0;
                            mode = ADDR_MODE_IMM;
                            if (imm > 0xFFFFFFFFLL || imm < -2147483648LL)
                                mode |= ADDR_MODE_IMM8;
                        } else {
                            int reg2 = check_reg(instr, t_op2, lineToTrim, srcCopy);
                            if (reg2 == -1) return 0;
                            src2 = (uint8_t) reg2;
                        }
                        int reg3 = check_reg(instr, t_op3, lineToTrim, srcCopy);
                        if (reg3 == -1) return 0;
                        dst = (uint8_t) reg3;
                    }
                    break;
                case INC:
                case DEC: {
                    int reg_inc = check_reg(instr, t_op1, lineToTrim, srcCopy);
                    if (reg_inc == -1) return 0;
                    dst = (uint8_t) reg_inc;
                    break;
                }
                case JMP:
                case CALL:
                    if (t_op1[0] == '$') {
                        imm = check_imm(instr, t_op1, &isImm, lineToTrim, srcCopy);
                        if (!isImm) return 0;
                        mode = ADDR_MODE_IMM;
                    } else if (t_op1[0] == '%') {
                        int reg = check_reg(instr, t_op1, lineToTrim, srcCopy);
                        if (reg == -1) return 0;
                        src1 = (uint8_t) reg;
                        src2 = src1;
                    } else {
                        imm = find_symbol(t_op1);
                        if (imm == -1) {
                            fprintf(stderr, "Error: Undefined symbol '%s' in %s\n", t_op1, instr);
                            free(lineToTrim);
                            free(srcCopy);
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
                        imm = check_imm(instr, t_op1, &isImm, lineToTrim, srcCopy);
                        if (!isImm) return 0;
                        mode = ADDR_MODE_IMM;
                    } else {
                        imm = find_symbol(t_op1);
                        if (imm == -1) {
                            fprintf(stderr, "Error: Undefined symbol '%s' in %s\n", t_op1, instr);
                            free(lineToTrim);
                            free(srcCopy);
                            return 0;
                        }
                        mode = ADDR_MODE_IMM;
                    }
                    break;
                case LOOP:
                    if (t_op1[0] == '$') {
                        imm = check_imm("LOOP", t_op1, &isImm, lineToTrim, srcCopy);
                        if (!isImm) return 0;
                        mode = ADDR_MODE_IMM;
                    } else {
                        imm = find_symbol(t_op1);
                        if (imm == -1) {
                            fprintf(stderr, "Error: Undefined symbol '%s' in LOOP\n", t_op1);
                            free(lineToTrim);
                            free(srcCopy);
                            return 0;
                        }
                        mode = ADDR_MODE_IMM;
                    }
                    int regJump = check_reg("LOOP", t_op2, lineToTrim, srcCopy);
                    if (regJump == -1) return 0;
                    dst = (uint8_t) regJump;
                    src1 = dst;
                    break;
                case CMP:
                    if (t_op1[0] == '$') {
                        imm = check_imm("CMP", t_op1, &isImm, lineToTrim, srcCopy);
                        if (!isImm) return 0;
                        mode = ADDR_MODE_IMM;
                        if (imm > 0xFFFFFFFFLL || imm < -2147483648LL) mode |= ADDR_MODE_IMM8;
                        int reg = check_reg("CMP", t_op2, lineToTrim, srcCopy);
                        if (reg == -1) return 0;
                        src1 = (uint8_t) reg;
                    } else {
                        int reg1 = check_reg("CMP", t_op1, lineToTrim, srcCopy);
                        if (reg1 == -1) return 0;
                        src1 = (uint8_t) reg1;
                        if (t_op2[0] == '$') {
                            imm = check_imm("CMP", t_op2, &isImm, lineToTrim, srcCopy);
                            if (!isImm) return 0;
                            mode = ADDR_MODE_IMM;
                            if (imm > 0xFFFFFFFFLL || imm < -2147483648LL)
                                mode |= ADDR_MODE_IMM8;
                        } else {
                            int reg2 = check_reg("CMP", t_op2, lineToTrim, srcCopy);
                            if (reg2 == -1) return 0;
                            src2 = (uint8_t) reg2;
                        }
                    }
                    break;
                case LOAD:
                    if (t_op1[0] == '$') {
                        imm = check_imm("LOAD", t_op1, &isImm, lineToTrim, srcCopy);
                        if (!isImm) return 0;
                        mode = ADDR_MODE_IMM;
                    } else {
                        int reg = check_reg("LOAD", t_op1, lineToTrim, srcCopy);
                        if (reg == -1) return 0;
                        src1 = (uint8_t) reg;
                    }
                    int regLoadDst = check_reg("LOAD", t_op2, lineToTrim, srcCopy);
                    if (regLoadDst == -1) return 0;
                    dst = (uint8_t) regLoadDst;
                    break;
                case STORE: {
                    int regStoreSrc = check_reg("STORE", t_op1, lineToTrim, srcCopy);
                    if (regStoreSrc == -1) return 0;
                    src1 = (uint8_t) regStoreSrc;
                    if (t_op2[0] == '$') {
                        imm = check_imm("STORE", t_op2, &isImm, lineToTrim, srcCopy);
                        if (!isImm) return 0;
                        mode = ADDR_MODE_IMM;
                    } else {
                        int reg2 = check_reg("STORE", t_op2, lineToTrim, srcCopy);
                        if (reg2 == -1) return 0;
                        src2 = (uint8_t) reg2;
                    }
                    dst = src1;
                    break;
                }
                case PUSH:
                    if (t_op1[0] == '$') {
                        imm = check_imm("PUSH", t_op1, &isImm, lineToTrim, srcCopy);
                        if (!isImm) return 0;
                        mode = ADDR_MODE_IMM;
                    } else {
                        int reg_push = check_reg("PUSH", t_op1, lineToTrim, srcCopy);
                        if (reg_push == -1) return 0;
                        src1 = (uint8_t) reg_push;
                    }
                    break;
                case POP: {
                    int regPopDst = check_reg("POP", t_op1, lineToTrim, srcCopy);
                    if (regPopDst == -1) return 0;
                    dst = (uint8_t) regPopDst;
                    break;
                }
                case SETZ:
                case TIME:
                case DEBUG: {
                    int regSetDst = check_reg(instr, t_op1, lineToTrim, srcCopy);
                    if (regSetDst == -1) return 0;
                    dst = (uint8_t) regSetDst;
                    break;
                }
                case RET:
                case EOP:
                    break;
                case EOPV:
                    if (t_op1[0] == '$') {
                        imm = check_imm("EOPV", t_op1, &isImm, lineToTrim, srcCopy);
                        if (!isImm) return 0;
                        mode = ADDR_MODE_IMM;
                    } else {
                        int reg_eopv = check_reg("EOPV", t_op1, lineToTrim, srcCopy);
                        if (reg_eopv == -1) return 0;
                        src1 = (uint8_t) reg_eopv;
                    }
                    break;
                case SYSCALL:
                    if (t_op1[0] == '$') {
                        imm = check_imm("SYSCALL", t_op1, &isImm, lineToTrim, srcCopy);
                        if (!isImm) return 0;
                        mode = ADDR_MODE_IMM;
                        if (imm > 0xFFFFFFFFLL || imm < -2147483648LL) mode |= ADDR_MODE_IMM8;
                    } else if (t_op1[0] == '%') {
                        int reg_syscall = check_reg("SYSCALL", t_op1, lineToTrim, srcCopy);
                        if (reg_syscall == -1) return 0;
                        src1 = (uint8_t) reg_syscall;
                    }
                    break;
                default: break;
            }

            size_t instrSize = 5;
            if (mode & ADDR_MODE_IMM) instrSize += (mode & ADDR_MODE_IMM8) ? 8 : 4;

            if (output && finalPos + instrSize > maxSize) {
                fprintf(stderr, "Error: Output buffer overflow, required size %zu, max size %zu\n", finalPos + instrSize, maxSize);
                free(lineToTrim);
                free(srcCopy);
                return 0;
            }

            encode_instruction((uint8_t)op, mode, dst, src1, src2, imm, output, &finalPos);
        }

        free(lineToTrim);
        line = strtok_r(NULL, "\n", &savePtr);
    }

    free(srcCopy);
    return finalPos;
}
