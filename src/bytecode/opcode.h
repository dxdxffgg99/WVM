#ifndef WVM_OPCODE_H
#define WVM_OPCODE_H

#include <stdint.h>

typedef enum opcode {
    NOP,  /* nop                    () */
    ADD,  /* add   dst1 src1 src2   (dst1 = src1 + src2) */
    ADDI, /* addi  dst1 src1 imm    (dst1 = src1 + imm1) */
    SUB,  /* sub   dst1 src1 src2   (dst1 = src1 - src2) */
    SUBI, /* subi  dst1 src1 imm    (dst1 = src1 - imm1) */
    MUL,  /* mul   dst1 src1 src2   (dst1 = src1 * src2) */
    MULI, /* muli  dst1 src1 imm    (dst1 = src1 * imm1) */
    DIV,  /* div   dst1 src1 src2   (dst1 = src1 / src2) */
    DIVI, /* divi  dst1 src1 imm    (dst1 = src1 / imm1) */
    JMP,  /* jmp   dst1             (goto dst1) */
    JMPI, /* jmpi  imm              (goto imm1) */
    CMP,  /* cmp   src1 src2        (rF = src1 == src2) */
    CMPI, /* cmpi  src1 imm         (rF = src1 == imm1) */
    JE,   /* je    imm              (if ZF==1: pc = imm) */
    JNE,  /* jne   imm              (if ZF==0: pc = imm) */
    JL,   /* jl    imm              (if SF==1: pc = imm) */
    JG,   /* jg    imm              (if ZF==0 && SF==0: pc = imm) */
    JLE,  /* jle   imm              (if ZF==1 || SF==1: pc = imm) */
    JGE,  /* jge   imm              (if SF==0: pc = imm) */
    SETZ, /* setz  dst1             (dst1 = rF) */
    COPY, /* copy  src1 src2        (src1 = src2) */
    LSH,  /* lsh   dst1 src1 src2   (src1 << src2) */
    LSHI, /* lshi  dst1 src1 imm1   (src1 << imm1) */
    RSH,  /* rsh   dst1 src1 src2   (src1 >> src2) */
    RSHI, /* rshi  dst1 src1 imm1   (src1 >> imm1) */
    LOAD, /* load  dst1 addr1 imm1  (dst = RAM[reg[base] + imm]) */
    STORE,/* store addr1 imm1 src1  (RAM[reg[base] + imm] = reg[src]) */
    OR,   /* or    dst src1 src2    (dst = src1 | src2) */
    ORI,  /* ori   dst1 src1 imm1   (dst1 = src1 | src2) */
    AND,  /* and   dst src1 src2    (dst1 = src1 & src2) */
    ANDI, /* andi  dst1 src1 imm1   (dst1 = src1 & imm1) */
    XOR,  /* xor   dst src1 src2    (dst1 = src1 ^ src2) */
    XORI, /* xori  dst1 src1 imm1   (dst1 = src1 ^ imm1) */
    TIME, /* time  dst1             (dst1 = time in ms since epoch) */
    RAND, /* rand  dst1 src1        (dst1 = random) */
    PUSH, /* push  src1             (stack push: stack[sp] = src1) */
    PUSHI,/* pushi imm1             (stack push: stack[sp] = imm1) */
    POP,  /* pop   dst1             (stack pop: dst1 = stack[sp]) */
    CALL, /* call  imm              (push return address, jump to imm) */
    RET,  /* ret                    (pop return address, jump back) */
    EOP,  /* eop                    (end of program with return 0) */
    EOPV, /* eop   src1             (end of program with return src1) */
    EOPVI,/* eop   imm1             (end of program with return imm1) */
    DEBUG /* debug                  (print ram, register value) */
} opcode_t;

#endif