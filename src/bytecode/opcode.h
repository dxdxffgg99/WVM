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
    CMP = 8,
    JE = 9,
    JNE = 10,
    JL = 11,
    JG = 12,
    JLE = 13,
    JGE = 14,
    SETZ = 15,
    MOV = 16,
    LSH = 17,
    RSH = 18,
    LOAD = 19,
    STORE = 20,
    OR = 21,
    AND = 22,
    XOR = 23,
    TIME = 24,
    PUSH = 25,
    POP = 26,
    CALL = 27,
    RET = 28,
    EOP = 29,
    EOPV = 30,
    DEBUG = 31,
    LOOP = 32,
    SYSCALL = 33
} opcode_t;

#endif
