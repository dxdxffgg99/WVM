#ifndef WVM_OPCODE_H
#define WVM_OPCODE_H

typedef enum opcode {
    NOP = 0,
    ADD = 1,
    SUB = 2,
    MUL = 3,
    DIV = 4,
    INC = 5,
    DEC = 6,
    JMP = 7,
    JE = 8,
    JNE = 9,
    JL = 10,
    JG = 11,
    JLE = 12,
    JGE = 13,
    SETZ = 14,
    MOV = 15,
    LSH = 16,
    RSH = 17,
    OR = 18,
    AND = 19,
    XOR = 20,
    CALL = 21,
    RET = 22,
    EOP = 23,
    EOPV = 24,
    LOOP = 25,
    SYSCALL = 26,
    PUSH = 27,
    POP = 28,
    LOAD = 29,
    STORE = 30,
    TIME = 31,
    DEBUG = 32
} opcode_t;

#endif
