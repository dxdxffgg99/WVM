#ifndef WVM_OPCODE_H
#define WVM_OPCODE_H

typedef enum opcode {
    NOP,  /* nop () [1] */
    ADD,  /* add dst1 src1 src2 (dst1 = src1 + src2) [4] */
    ADDI, /* addi dst1 src1 imm (dst1 = src1 + imm1) [4] */
    SUB,  /* sub dst1 src1 src2 (dst1 = src1 - src2) [4] */
    SUBI, /* subi dst1 src1 imm (dst1 = src1 - imm1) [4] */
    MUL,  /* mul dst1 src1 src2 (dst1 = src1 * src2) [4] */
    MULI, /* muli dst1 src1 imm (dst1 = src1 * imm1) [4] */
    DIV,  /* div dst1 src1 src2 (dst1 = src1 / src2) [4] */
    DIVI, /* divi dst1 src1 imm (dst1 = src1 / imm1) [4] */
    JMP,  /* jmp dst1 (goto dst1) [NaN] */
    JMPI, /* jmpi imm (goto imm1) [NaN] */
    CMP,  /* cmp src1 src2 (src1 == src2) [3] */
    CMPI, /* cmpi src1 imm (src1 == imm1) [3] */
    COPY, /* copy src1 src2 (src1 = src2) [3] */
    LSH,  /* lsh src1 src2 (src1 << src2) [3] */
    LSHI, /* lshi src1 imm1 (src1 << imm1) [3] */
    RSH,  /* rsh src1 src2 (src1 >> src2) [3] */
    RSHI, /* rshi src1 imm1 (src1 >> imm1) [3] */
    LOAD, /* load dst1 addr1 (dst1 = ram[addr1]) [3] */
    STORE,/* store addr1 src1 (ram[addr1] = src1) [3] */
    OR,   /* or dst src1 src2 (dst1 = src1 | src2) [4] */
    AND,  /* and dst src1 src2 (dst1 = src1 & src2) [4] */
    XOR,  /* xor dst src1 src2 (dst1 = src1 ^ src2) [4] */
    ORI,  /* or dst1 src1 imm1 (dst1 = src1 | src2) [4] */
    ANDI, /* and dst1 src1 imm1 (dst1 = src1 & imm1) [4] */
    XORI, /* xor dst1 src1 imm1 (dst1 = src1 ^ imm1) [4] */
    EOP,  /* eop (end of program with return 0) [NaN] */
    EOPV, /* eop src1 (end of program with return src1) [NaN] */
    EOPVI,/* eop imm1 (end of program with return imm1) [NaN] */
} opcode_t;

#endif