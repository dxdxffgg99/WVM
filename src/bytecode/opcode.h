#ifndef WVM_OPCODE_H
#define WVM_OPCODE_H

#include <stdint.h>

typedef enum opcode {
    NOP,
    ADD,
    SUB,
    MUL,
    DIV,
    INC,
    DEC,
    JMP,
    CMP,
    JE,
    JNE,
    JL,
    JG,
    JLE,
    JGE,
    SETZ,
    MOV,
    LSH,
    RSH,
    LOAD,
    STORE,
    OR,
    AND,
    XOR,
    TIME,
    PUSH,
    POP,
    CALL,
    RET,
    EOP,
    EOPV,
    DEBUG
} opcode_t;

#endif