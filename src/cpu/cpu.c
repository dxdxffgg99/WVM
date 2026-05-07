#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "../bytecode/opcode.h"

_Pragma("GCC diagnostic push")
_Pragma("GCC diagnostic ignored \"-Wpedantic\"")

int64_t run(CPU *cpu) {
    static void *dispatch[] = {
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
        &&OP_DEBUG,
        &&OP_LOOP,
        &&OP_SYSCALL
    };

    if (!cpu->isThreaded) {
        for (ram_addr_t i = 0; i < cpu->decodedSize; i++) {
            cpu->decodedProgram[i].handler = dispatch[cpu->decodedProgram[i].opcode];
        }
        cpu->isThreaded = true;
    }

    cpu->running = true;

    instr_t *p_ins = &cpu->decodedProgram[0];
    const Registers *regs = &cpu->regs;
    Registers *m_regs = &cpu->regs;
    const uint8_t *ram_data = cpu->ram.data;
    uint8_t *m_ram_data = cpu->ram.data;

#define NEXT() do { \
    p_ins++; \
    if (p_ins >= cpu->decodedProgram + cpu->decodedSize) { \
        cpu->running = false; \
        cpu->returnValue = 0; \
        goto FINAL; \
    } \
    goto *p_ins->handler; \
} while (0)

#define JUMP(target_addr) do { \
    if (p_ins->jump_target) { \
        p_ins = p_ins->jump_target; \
    } else { \
        uint64_t _addr = (target_addr); \
        ram_addr_t _pci = 0; \
        while (_pci < cpu->decodedSize && cpu->decodedProgram[_pci].addr < _addr) { \
            _pci++; \
        } \
        p_ins = &cpu->decodedProgram[_pci]; \
    } \
    goto *p_ins->handler; \
} while (0)

#define GET_SRC1 ((p_ins->mode & ADDR_MODE_IMM) ? (p_ins->imm) : (regs->registers[p_ins->src1]))
#define GET_SRC2 ((p_ins->mode & ADDR_MODE_IMM) ? (p_ins->imm) : (regs->registers[p_ins->src2]))
#define READ_REG(reg) regs->registers[(reg)]
#define WRITE_REG(reg, val) m_regs->registers[(reg)] = (val)

#define BINARY_OP(op) do { \
    WRITE_REG(p_ins->dst, READ_REG(p_ins->src1) op GET_SRC2); \
    NEXT(); \
} while (0)

#define CHECK_MEM(addr, len) do { \
    uint64_t _addr = (uint64_t)(addr); \
    uint64_t _len = (uint64_t)(len); \
    if (_addr + _len > cpu->ram.size) { \
        cpu->running = false; \
        cpu->returnValue = -1; \
        goto FINAL; \
    } \
} while (0)

    goto *p_ins->handler;

OP_NOP: {
        NEXT();
    }

OP_ADD: {
        BINARY_OP(+);
    }

OP_SUB: {
        BINARY_OP(-);
    }

OP_MUL: {
        BINARY_OP(*);
    }

OP_DIV: {
        const int64_t divisor = GET_SRC2;
        if (divisor == 0) {
            cpu->running = false;
            cpu->returnValue = -1;
            goto FINAL;
        }

        WRITE_REG(p_ins->dst, READ_REG(p_ins->src1) / divisor);
        NEXT();
    }

OP_INC: {
        WRITE_REG(p_ins->dst, READ_REG(p_ins->dst) + 1);
        NEXT();
    }

OP_DEC: {
        WRITE_REG(p_ins->dst, READ_REG(p_ins->dst) - 1);
        NEXT();
    }

OP_JMP: {
        JUMP(GET_SRC1);
    }

OP_CMP: {
        const int64_t a = READ_REG(p_ins->src1);
        const int64_t b = GET_SRC2;
        const int64_t r = a - b;

        cpu->regs.zero_flag = (r == 0);
        cpu->regs.sign_flag = (r < 0);
        cpu->regs.carry_flag = ((uint64_t) a < (uint64_t) b);

        NEXT();
    }

OP_JE: {
        if (cpu->regs.zero_flag) {
            JUMP(GET_SRC1);
        }
        NEXT();
    }

OP_JNE: {
        if (!cpu->regs.zero_flag) {
            JUMP(GET_SRC1);
        }
        NEXT();
    }

OP_JL: {
        if (cpu->regs.sign_flag) {
            JUMP(GET_SRC1);
        }
        NEXT();
    }

OP_JG: {
        if (!cpu->regs.sign_flag && !cpu->regs.zero_flag) {
            JUMP(GET_SRC1);
        }
        NEXT();
    }

OP_JLE: {
        if (cpu->regs.sign_flag || cpu->regs.zero_flag) {
            JUMP(GET_SRC1);
        }
        NEXT();
    }

OP_JGE: {
        if (!cpu->regs.sign_flag) {
            JUMP(GET_SRC1);
        }
        NEXT();
    }

OP_SETZ: {
        WRITE_REG(p_ins->dst, (int64_t)cpu->regs.zero_flag);
        NEXT();
    }

OP_MOV: {
        WRITE_REG(p_ins->dst, GET_SRC1);
        NEXT();
    }

OP_LSH: {
        WRITE_REG(p_ins->dst, (int64_t)((uint64_t)READ_REG(p_ins->src1) << (GET_SRC2 & 63)));
        NEXT();
    }

OP_RSH: {
        WRITE_REG(p_ins->dst, (int64_t)((uint64_t)READ_REG(p_ins->src1) >> (GET_SRC2 & 63)));
        NEXT();
    }

OP_LOAD: {
        CHECK_MEM(GET_SRC1, 8);
        WRITE_REG(p_ins->dst, *(int64_t*)&ram_data[GET_SRC1]);
        NEXT();
    }

OP_STORE: {
        CHECK_MEM(GET_SRC2, 8);
        *(int64_t *) &m_ram_data[GET_SRC2] = READ_REG(p_ins->dst);
        NEXT();
    }

OP_OR: {
        BINARY_OP(|);
    }

OP_AND: {
        BINARY_OP(&);
    }

OP_XOR: {
        BINARY_OP(^);
    }

OP_TIME: {
        WRITE_REG(p_ins->dst, (int64_t)time(NULL));
        NEXT();
    }

OP_PUSH: {
        if (m_regs->stack_pointer < 8) {
            cpu->running = false;
            cpu->returnValue = -1;
            goto FINAL;
        }
        m_regs->stack_pointer -= 8;
        *(int64_t *)&m_ram_data[m_regs->stack_pointer] = GET_SRC1;
        NEXT();
    }

OP_POP: {
        CHECK_MEM(regs->stack_pointer, 8);
        WRITE_REG(p_ins->dst, *(int64_t*)&ram_data[regs->stack_pointer]);
        m_regs->stack_pointer += 8;
        NEXT();
    }

OP_CALL: {
        if (m_regs->stack_pointer < 8) {
            cpu->running = false;
            cpu->returnValue = -1;
            goto FINAL;
        }

        uint64_t ret_index = (uint64_t)(p_ins - cpu->decodedProgram + 1);

        m_regs->stack_pointer -= 8;
        *(uint64_t *)&m_ram_data[m_regs->stack_pointer] = ret_index;

        JUMP(GET_SRC1);
    }

OP_RET: {
        CHECK_MEM(regs->stack_pointer, 8);

        uint64_t ret = *(uint64_t *)&ram_data[regs->stack_pointer];
        m_regs->stack_pointer += 8;

        if (ret >= cpu->decodedSize) {
            cpu->running = false;
            cpu->returnValue = -1;
            goto FINAL;
        }

        p_ins = &cpu->decodedProgram[ret];
        goto *p_ins->handler;
    }

OP_EOP: {
        cpu->running = false;
        cpu->returnValue = 0;
        goto FINAL;
    }

OP_EOPV: {
        cpu->running = false;
        cpu->returnValue = READ_REG(p_ins->src1);
        goto FINAL;
    }

OP_DEBUG: {
        cpu_dump_registers(cpu);
        NEXT();
    }

OP_LOOP: {
        const int64_t val = READ_REG(p_ins->dst) - 1;
        WRITE_REG(p_ins->dst, val);
        if (val > 0) {
            JUMP(GET_SRC1);
        }
        NEXT();
    }

OP_SYSCALL: {
         const int64_t syscall_num = GET_SRC1;

         switch (syscall_num) {
             case 0:
                 if (m_regs->stack_pointer < 8) {
                     cpu->running = false;
                     cpu->returnValue = -1;
                     goto FINAL;
                 }
                 m_regs->stack_pointer -= 8;
                 *(int64_t *) &m_ram_data[regs->stack_pointer] = READ_REG(0);
                 break;
             case 1:
                 if (regs->stack_pointer + 8 > cpu->ram.size) {
                     cpu->running = false;
                     cpu->returnValue = -1;
                     goto FINAL;
                 }
                 WRITE_REG(0, *(int64_t*)&ram_data[regs->stack_pointer]);
                 m_regs->stack_pointer += 8;
                 break;
             case 2:
                 printf("%ld", READ_REG(0));
                 break;
             case 3: {
                 int64_t ch = READ_REG(0);
                 if (ch >= 0 && ch <= 127) {
                     printf("%c", (char)ch);
                 } else if (ch >= 128 && ch <= 0x7FF) {
                     char utf8[3];

                     utf8[0] = (char)(0xC0 | (ch >> 6));
                     utf8[1] = (char)(0x80 | (ch & 0x3F));
                     utf8[2] = '\0';

                     printf("%s", utf8);
                 } else if (ch >= 0x800 && ch <= 0xFFFF) {
                     char utf8[4];

                     utf8[0] = (char)(0xE0 | (ch >> 12));
                     utf8[1] = (char)(0x80 | ((ch >> 6) & 0x3F));
                     utf8[2] = (char)(0x80 | (ch & 0x3F));
                     utf8[3] = '\0';

                     printf("%s", utf8);
                 } else if (ch >= 0x10000 && ch <= 0x10FFFF) {
                     char utf8[5];

                     utf8[0] = (char)(0xF0 | (ch >> 18));
                     utf8[1] = (char)(0x80 | ((ch >> 12) & 0x3F));
                     utf8[2] = (char)(0x80 | ((ch >> 6) & 0x3F));
                     utf8[3] = (char)(0x80 | (ch & 0x3F));
                     utf8[4] = '\0';

                     printf("%s", utf8);
                 }
                 break;
             }
             case 4: {
                 uint64_t addr = READ_REG(1);

                 CHECK_MEM(addr, 8);

                 int64_t tmp;
                 memcpy(&tmp, &cpu->ram.data[addr], 8);

                 WRITE_REG(0, tmp);
                 break;
             }
             case 5: {
                 uint64_t addr = READ_REG(1);
                 int64_t val = READ_REG(2);

                 CHECK_MEM(addr, 8);

                 *(int64_t *)&m_ram_data[addr] = val;

                 break;
             }
             case 6: {
                 WRITE_REG(0, (int64_t)getchar());

                 break;
             }
             case 9: {
                 printf("\n");

                 break;
             }
             case 10: {
                 printf("Hash: %lx\n", READ_REG(0));
                 break;
             }
             case 11: {
                 int64_t val = READ_REG(0);

                 printf("%ld (0x%lx)\n", val, val);

                 break;
             }

             case 12: {
                 printf("%lx\n", READ_REG(0));

                 break;
             }
             case 13: {
                 int64_t val = READ_REG(0);

                 for (int i = 63; i >= 0; i--) {
                     printf("%d", (int)((val >> i) & 1));
                 }

                 printf("\n");

                 break;
             }
             case 14: {
                 printf("RAM Size: %lu bytes\n", cpu->ram.size);
                 printf("Stack Pointer: %lu\n", regs->stack_pointer);

                 break;
             }
             case 15: {
                 uint64_t dest = READ_REG(1);
                 uint64_t src = READ_REG(2);
                 int64_t len = READ_REG(3);

                 if (len < 0) {
                     cpu->running = false;
                     cpu->returnValue = -1;
                     goto FINAL;
                 }

                 CHECK_MEM(dest, (size_t)len);
                 CHECK_MEM(src, (size_t)len);
                 memcpy(&m_ram_data[dest], &ram_data[src], (size_t)len);

                 break;
             }
             case 16: {
                 uint64_t addr = READ_REG(1);
                 int64_t len = READ_REG(3);

                 if (len < 0) {
                     cpu->running = false;
                     cpu->returnValue = -1;
                     goto FINAL;
                 }

                 CHECK_MEM(addr, (size_t)len);
                 memset(&m_ram_data[addr], (int)READ_REG(2), (size_t)len);

                 break;
             }
             case 17: {
                 int64_t us = READ_REG(0);

                 if (us < 0) {
                     cpu->running = false;
                     cpu->returnValue = -1;
                     goto FINAL;
                 }

                 struct timespec req;
                 req.tv_sec = us / 1000000;
                 req.tv_nsec = (us % 1000000) * 1000;

                 nanosleep(&req, NULL);
                 break;
             }
             default:
                 cpu->running = false;
                 cpu->returnValue = -1;
                 goto FINAL;
         }
         NEXT();
     }

FINAL:
#undef NEXT
#undef JUMP

    return cpu->returnValue;
}

_Pragma("GCC diagnostic pop")

bool instr_decode(const uint8_t *buffer, size_t limit, instr_t *instr) {
    if (limit < INSTR_MIN_SIZE) return false;
    instr->opcode = buffer[0];
    instr->mode = buffer[1];
    instr->dst = buffer[2];
    instr->src1 = buffer[3];
    instr->src2 = buffer[4];
    instr->size = INSTR_MIN_SIZE;

    if (instr->mode & ADDR_MODE_IMM) {
        instr->imm = 0;
        int imm_size = (instr->mode & ADDR_MODE_IMM8) ? 8 : 4;
        if (limit < (size_t) (INSTR_MIN_SIZE + imm_size)) return false;
        for (int i = 0; i < imm_size; i++) {
            instr->imm |= ((imm_t) buffer[5 + i]) << (8 * i);
        }

        if (imm_size == 4) {
            if (instr->imm & 0x80000000) {
                instr->imm |= 0xFFFFFFFF00000000;
            }
        }
        instr->size += imm_size;
    } else {
        instr->imm = 0;
    }
    return true;
}

ram_addr_t instr_array_size(const ram_addr_t instr_count) {
    return instr_count * INSTR_MAX_SIZE;
}

int cpu_init(CPU *cpu, const ram_addr_t ram_size) {
    if (!(cpu && ram_size)) {
        return 1;
    }

    cpu->ram.data = (uint8_t *) malloc(ram_size);
    if (!cpu->ram.data) {
        return 1;
    }

    cpu->ram.size = ram_size;
    memset(cpu->ram.data, 0, ram_size);

    memset(&cpu->regs, 0, sizeof(Registers));
    cpu->regs.stack_pointer = ram_size;

    cpu->pc = 0;
    cpu->decodedProgram = NULL;
    cpu->decodedSize = 0;
    cpu->running = false;
    cpu->isThreaded = false;
    cpu->returnValue = 0;

    return 0;
}

void cpu_free(CPU *cpu) {
    if (cpu) {
        if (cpu->ram.data) {
            free(cpu->ram.data);
            cpu->ram.data = NULL;
            cpu->ram.size = 0;
        }
        if (cpu->decodedProgram) {
            free(cpu->decodedProgram);
            cpu->decodedProgram = NULL;
            cpu->decodedSize = 0;
        }
    }
}

void load_program(CPU *cpu, const uint8_t *code, const ram_addr_t size) {
    if (size > cpu->ram.size) {
        return;
    }
    memcpy(cpu->ram.data, code, size);
    cpu->pc = 0;
    cpu->isThreaded = false;

    if (cpu->decodedProgram) {
        free(cpu->decodedProgram);
        cpu->decodedProgram = NULL;
    }

    cpu->decodedProgram = (instr_t *) malloc(sizeof(instr_t) * (size / 5 + 1));
    cpu->decodedSize = 0;

    ram_addr_t offset = 0;
    while (offset < size) {
        instr_t ins = {0};
        if (!instr_decode(cpu->ram.data + offset, size - offset, &ins)) {
            break;
        }
        ins.addr = offset;
        ins.jump_pci = -1;
        cpu->decodedProgram[cpu->decodedSize++] = ins;
        offset += ins.size;
    }

    for (ram_addr_t i = 0; i < cpu->decodedSize; i++) {
        instr_t *ins = &cpu->decodedProgram[i];
        switch (ins->opcode) {
            case JMP:
            case JE:
            case JNE:
            case JL:
            case JG:
            case JLE:
            case JGE:
            case CALL:
            case LOOP:
                if (ins->mode & ADDR_MODE_IMM) {
                    uint64_t target = (uint64_t) ins->imm;
                    for (ram_addr_t j = 0; j < cpu->decodedSize; j++) {
                        if (cpu->decodedProgram[j].addr == target) {
                            ins->jump_pci = (int32_t) j;
                            ins->jump_target = &cpu->decodedProgram[j];
                            break;
                        }
                    }
                }
                break;
            default:
                break;
        }
    }}

void cpu_dump_registers(const CPU *cpu) {
    printf("============================================================= REGISTER ============================================================================\n");

    for (int base = 0; base < REG_COUNT; base += 8) {
        printf("R%02d | ", base);
        for (int i = 0; i < 8 && (base + i) < REG_COUNT; i++) {
            printf("%16ld ", reg_read(&cpu->regs, (int64_t)(base + i)));
        }
        printf("\n");
    }

    printf("============================================================ DEBUG END ============================================================================\n");
}
