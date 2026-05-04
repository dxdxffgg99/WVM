#include "assembler.h"
#include "../bytecode/opcode.h"
#include "../cpu/cpu.h"
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

typedef struct {
        const char *name;
        opcode_t op;
} OpMapping;

static const OpMapping op_table[] = {
        {"nop", NOP}, {"add", ADD}, {"sub", SUB}, {"mul", MUL}, {"div", DIV},
        {"inc", INC}, {"dec", DEC}, {"jmp", JMP}, {"cmp", CMP}, {"je", JE},
        {"jne", JNE}, {"jl", JL}, {"jg", JG}, {"jle", JLE}, {"jge", JGE},
        {"setz", SETZ}, {"mov", MOV}, {"lsh", LSH}, {"rsh", RSH}, {"load", LOAD},
        {"store", STORE}, {"or", OR}, {"and", AND}, {"xor", XOR}, {"time", TIME},
        {"push", PUSH}, {"pop", POP}, {"call", CALL}, {"ret", RET}, {"eop", EOP},
        {"eopv", EOPV}, {"debug", DEBUG}, {"loop", LOOP},
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
        if (str[0] != '%') return -1;
        if (str[1] == 'r') {
                return atoi(&str[2]);
        }
        return -1;
}

static int64_t parse_immediate(const char *str, bool *is_imm) {
        if (str[0] == '$') {
                *is_imm = true;
                return strtoll(&str[1], NULL, 0);
        }
        *is_imm = false;
        return 0;
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
        size_t pos = 0;

        char *saveptr;
        char *line = strtok_r(src_copy, "\n", &saveptr);
        while (line != NULL) {
                char *line_to_trim = strdup(line);
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
                        add_symbol(trim(trimmed), pos);
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
                        if (op != -1) {
                                uint8_t mode = 0;
                                char *ops[3] = {trim(op1), trim(op2), trim(op3)};
                                for (int i = 0; i < 3; i++) {
                                        if (ops[i][0] == '$') {
                                                bool is_imm;
                                                int64_t imm = parse_immediate(ops[i], &is_imm);
                                                if (is_imm) {
                                                        mode |= ADDR_MODE_IMM;
                                                        if (imm > 0xFFFFFFFFLL || imm < -2147483648LL)
                                                                mode |= ADDR_MODE_IMM8;
                                                }
                                                break;
                                        } else if (ops[i][0] != '\0' && ops[i][0] != '%' &&
                                                   op != NOP && op != RET && op != EOP) {
                                                mode |= ADDR_MODE_IMM;
                                                break;
                                        }
                                }
                                encode_instruction((uint8_t) op, mode, 0, 0, 0, 0, NULL, &pos);
                        }
                }
                free(line_to_trim);
                line = strtok_r(NULL, "\n", &saveptr);
        }
        free(src_copy);

        src_copy = strdup(source);
        size_t final_pos = 0;
        line = strtok_r(src_copy, "\n", &saveptr);
        while (line != NULL) {
                char *line_to_trim = strdup(line);
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
                                                mode = ADDR_MODE_IMM;
                                                if (imm > 0xFFFFFFFFLL || imm < -2147483648LL) mode |= ADDR_MODE_IMM8;
                                                dst = (uint8_t) parse_register(t_op2);
                                        } else {
                                                src1 = (uint8_t) parse_register(t_op1);
                                                dst = (uint8_t) parse_register(t_op2);
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
                                                mode = ADDR_MODE_IMM;
                                                if (imm > 0xFFFFFFFFLL || imm < -2147483648LL) mode |= ADDR_MODE_IMM8;
                                                dst = (uint8_t) parse_register(t_op2);
                                                src1 = dst;
                                        } else if (count == 3) {
                                                src2 = (uint8_t) parse_register(t_op1);
                                                dst = (uint8_t) parse_register(t_op2);
                                                src1 = dst;
                                        } else if (count == 4) {
                                                src1 = (uint8_t) parse_register(t_op1);
                                                if (t_op2[0] == '$') {
                                                        imm = parse_immediate(t_op2, &is_imm);
                                                        mode = ADDR_MODE_IMM;
                                                        if (imm > 0xFFFFFFFFLL || imm < -2147483648LL)
                                                                mode |= ADDR_MODE_IMM8;
                                                } else {
                                                        src2 = (uint8_t) parse_register(t_op2);
                                                }
                                                dst = (uint8_t) parse_register(t_op3);
                                        }
                                        break;
                                case INC:
                                case DEC:
                                        dst = (uint8_t) parse_register(t_op1);
                                        break;
                                case JMP:
                                case CALL:
                                        if (t_op1[0] == '$') {
                                                imm = parse_immediate(t_op1, &is_imm);
                                                mode = ADDR_MODE_IMM;
                                        } else if (t_op1[0] == '%') {
                                                src1 = (uint8_t) parse_register(t_op1);
                                                src2 = src1;
                                        } else {
                                                imm = find_symbol(t_op1);
                                                mode = ADDR_MODE_IMM;
                                        }
                                        break;
                                case JE:
                                case JNE:
                                case JL:
                                case JG:
                                case JLE:
                                case JGE:
                                case LOOP:
                                        if (t_op1[0] == '$') {
                                                imm = parse_immediate(t_op1, &is_imm);
                                                mode = ADDR_MODE_IMM;
                                        } else {
                                                imm = find_symbol(t_op1);
                                                mode = ADDR_MODE_IMM;
                                        }
                                        dst = (uint8_t) parse_register(t_op2);
                                        src1 = dst;
                                        break;
                                case CMP:
                                        if (t_op1[0] == '$') {
                                                imm = parse_immediate(t_op1, &is_imm);
                                                mode = ADDR_MODE_IMM;
                                                if (imm > 0xFFFFFFFFLL || imm < -2147483648LL) mode |= ADDR_MODE_IMM8;
                                                src1 = (uint8_t) parse_register(t_op2);
                                        } else {
                                                src1 = (uint8_t) parse_register(t_op1);
                                                if (t_op2[0] == '$') {
                                                        imm = parse_immediate(t_op2, &is_imm);
                                                        mode = ADDR_MODE_IMM;
                                                        if (imm > 0xFFFFFFFFLL || imm < -2147483648LL)
                                                                mode |= ADDR_MODE_IMM8;
                                                } else {
                                                        src2 = (uint8_t) parse_register(t_op2);
                                                }
                                        }
                                        break;
                                case LOAD:
                                        if (t_op1[0] == '$') {
                                                imm = parse_immediate(t_op1, &is_imm);
                                                mode = ADDR_MODE_IMM;
                                        } else {
                                                src1 = (uint8_t) parse_register(t_op1);
                                        }
                                        dst = (uint8_t) parse_register(t_op2);
                                        break;
                                case STORE:
                                        src1 = (uint8_t) parse_register(t_op1);
                                        if (t_op2[0] == '$') {
                                                imm = parse_immediate(t_op2, &is_imm);
                                                mode = ADDR_MODE_IMM;
                                        } else {
                                                src2 = (uint8_t) parse_register(t_op2);
                                        }
                                        dst = src1;
                                        break;
                                case PUSH:
                                        if (t_op1[0] == '$') {
                                                imm = parse_immediate(t_op1, &is_imm);
                                                mode = ADDR_MODE_IMM;
                                        } else {
                                                src1 = (uint8_t) parse_register(t_op1);
                                        }
                                        break;
                                case POP:
                                        dst = (uint8_t) parse_register(t_op1);
                                        break;
                                case SETZ:
                                case TIME:
                                case DEBUG:
                                        dst = (uint8_t) parse_register(t_op1);
                                        break;
                                case RET:
                                case EOP:
                                        break;
                                case EOPV:
                                        if (t_op1[0] == '$') {
                                                imm = parse_immediate(t_op1, &is_imm);
                                                mode = ADDR_MODE_IMM;
                                        } else {
                                                src1 = (uint8_t) parse_register(t_op1);
                                        }
                                        break;
                                default: break;
                        }

                        if (output && final_pos + INSTR_MAX_SIZE <= max_size) {
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
