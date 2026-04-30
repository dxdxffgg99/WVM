#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

uint64_t run(CPU* cpu) {
    cpu->running = 1;

#define FETCH() \
ins = (const instr_t*)(cpu->ram.data + cpu->pc);

#define NEXT() do { \
cpu->pc += 13; \
FETCH(); \
goto *dispatch[ins->opcode]; \
} while (0)

#define JUMP(addr) do { \
cpu->pc = (addr); \
FETCH(); \
goto *dispatch[ins->opcode]; \
} while (0)

    static void* dispatch[] = {
        &&OP_NOP,
        &&OP_ADD,
        &&OP_SUB,
        &&OP_MUL,
        &&OP_DIV,

        &&OP_INC,
        &&OP_DEC,

        &&OP_JMP,
        &&OP_CMP,

        &&OP_JE,
        &&OP_JNE,
        &&OP_JL,
        &&OP_JG,
        &&OP_JLE,
        &&OP_JGE,

        &&OP_SETZ,

        &&OP_MOV,

        &&OP_LSH,
        &&OP_RSH,

        &&OP_LOAD,
        &&OP_STORE,

        &&OP_OR,
        &&OP_AND,
        &&OP_XOR,

        &&OP_TIME,

        &&OP_PUSH,
        &&OP_POP,

        &&OP_CALL,
        &&OP_RET,

        &&OP_EOP,
        &&OP_EOPV,

        &&OP_DEBUG
    };

    const instr_t* ins;

    FETCH();
    goto *dispatch[ins->opcode];

    OP_NOP: { NEXT(); }

    OP_ADD: {
        reg_write(&cpu->regs,
            ins->dst,
            reg_read(&cpu->regs, ins->src1) +
            (ins->mode ? ins->imm : reg_read(&cpu->regs, ins->src2))
        );
        NEXT();
    }

    OP_SUB: {
        reg_write(&cpu->regs,
            ins->dst,
            reg_read(&cpu->regs, ins->src1) -
            (ins->mode ? ins->imm : reg_read(&cpu->regs, ins->src2))
        );
        NEXT();
    }

    OP_MUL: {
        reg_write(&cpu->regs,
            ins->dst,
            reg_read(&cpu->regs, ins->src1) *
            (ins->mode ? ins->imm : reg_read(&cpu->regs, ins->src2))
        );
        NEXT();
    }

    OP_DIV: {
        assert(
            (ins->mode ? ins->imm : reg_read(&cpu->regs, ins->src2)) != 0
        );

        reg_write(&cpu->regs,
            ins->dst,
            reg_read(&cpu->regs, ins->src1) /
            (ins->mode ? ins->imm : reg_read(&cpu->regs, ins->src2))
        );
        NEXT();
    }

    OP_INC: {
        reg_write(&cpu->regs, ins->dst,
            reg_read(&cpu->regs, ins->dst) + 1);
        NEXT();
    }

    OP_DEC: {
        reg_write(&cpu->regs, ins->dst,
            reg_read(&cpu->regs, ins->dst) - 1);
        NEXT();
    }

    OP_JMP: { JUMP(get_src1); }

    OP_CMP: {
        const int64_t a = reg_read(&cpu->regs, ins->src1);
        const int64_t b = get_src2;
        const int64_t r = a - b;

        cpu->regs.zero_flag = (r == 0);
        cpu->regs.sign_flag = (r < 0);
        cpu->regs.carry_flag = (a < b);

        NEXT();
    }

    OP_JE: { if (cpu->regs.zero_flag) JUMP(get_src1); NEXT(); }
    OP_JNE: { if (!cpu->regs.zero_flag) JUMP(get_src1); NEXT(); }
    OP_JL: { if (cpu->regs.sign_flag) JUMP(get_src1); NEXT(); }
    OP_JG: { if (!cpu->regs.sign_flag && !cpu->regs.zero_flag) JUMP(get_src1); NEXT(); }
    OP_JLE: { if (cpu->regs.sign_flag || cpu->regs.zero_flag) JUMP(get_src1); NEXT(); }
    OP_JGE: { if (!cpu->regs.sign_flag) JUMP(get_src1); NEXT(); }

    OP_SETZ: {
        reg_write(&cpu->regs, ins->dst, (int64_t)cpu->regs.zero_flag);
        NEXT();
    }

    OP_MOV: {
        reg_write(&cpu->regs, ins->dst, get_src1);
        NEXT();
    }

    OP_LSH: {
        reg_write(&cpu->regs, ins->dst,
            (int64_t)((uint64_t)reg_read(&cpu->regs, ins->src1) << (get_src2 & 63)));
        NEXT();
    }

    OP_RSH: {
        reg_write(&cpu->regs, ins->dst,
            (int64_t)((uint64_t)reg_read(&cpu->regs, ins->src1) >> (get_src2 & 63)));
        NEXT();
    }

    OP_LOAD: {
        reg_write(&cpu->regs, ins->dst, mem_read64(&cpu->ram, get_src1));
        NEXT();
    }

    OP_STORE: {
        mem_write64(&cpu->ram, get_src1, reg_read(&cpu->regs, ins->dst));
        NEXT();
    }

    OP_OR: {
        reg_write(&cpu->regs,
            ins->dst,
            reg_read(&cpu->regs, ins->src1) |
            (ins->mode ? ins->imm : reg_read(&cpu->regs, ins->src2))
        );
        NEXT();
    }

    OP_AND: {
        reg_write(&cpu->regs,
            ins->dst,
            reg_read(&cpu->regs, ins->src1) &
            (ins->mode ? ins->imm : reg_read(&cpu->regs, ins->src2))
        );
        NEXT();
    }

    OP_XOR: {
        reg_write(&cpu->regs,
            ins->dst,
            reg_read(&cpu->regs, ins->src1) ^
            (ins->mode ? ins->imm : reg_read(&cpu->regs, ins->src2))
        );
        NEXT();
    }

    OP_TIME: {
        reg_write(&cpu->regs, ins->dst, (int64_t)time(NULL));
        NEXT();
    }

    OP_PUSH: {
        cpu->regs.stack_pointer -= 8;
        mem_write64(&cpu->ram, cpu->regs.stack_pointer, get_src1);
        NEXT();
    }

    OP_POP: {
        reg_write(&cpu->regs, ins->dst,
            mem_read64(&cpu->ram, cpu->regs.stack_pointer));
        cpu->regs.stack_pointer += 8;
        NEXT();
    }

    OP_CALL: {
        cpu->regs.stack_pointer -= 8;
        mem_write64(&cpu->ram, cpu->regs.stack_pointer, (int64_t)cpu->pc + 13);
        JUMP((ins->mode) ? ins->imm : reg_read(&cpu->regs, ins->src2));
    }

    OP_RET: {
        cpu->pc = (uint64_t)mem_read64(&cpu->ram, cpu->regs.stack_pointer);
        cpu->regs.stack_pointer += 8;
        FETCH();
        goto *dispatch[((instr_t*)(cpu->ram.data + cpu->pc))->opcode];
    }

    OP_EOP: {
        cpu->running = 0;
        cpu->rv = 0;
        goto FINAL;
    }

    OP_EOPV: {
        cpu->running = 0;
        cpu->rv = (ins->mode) ? (int)ins->imm : (int)reg_read(&cpu->regs, ins->src1);
        goto FINAL;
    }

    OP_DEBUG: {
        printf("\n");
        printf("==============================================================================================================================================\n");
        printf("============================================================= REGISTER =======================================================================\n");
        printf("==============================================================================================================================================\n");

        for (int base = 0; base < REG_COUNT; base += 8) {
            printf("R%03d | ", base);

            for (int i = 0; i < 8 && (base + i) < REG_COUNT; i++) {
                printf("%16ld ", reg_read(&cpu->regs, base + i));
            }

            printf("\n");
        }

        printf("==============================================================================================================================================\n");
        printf("============================================================ DEBUG END =======================================================================\n");
        printf("==============================================================================================================================================\n");

        NEXT();
    }


FINAL:
    free(cpu->ram.data);
    cpu->ram.data = NULL;
    cpu->ram.size = 0;

#undef NEXT
#undef JUMP
#undef FETCH

    return cpu->rv;
}