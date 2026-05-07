#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "../bytecode/opcode.h"

_Pragma("GCC diagnostic push")
_Pragma("GCC diagnostic ignored \"-Wpedantic\"")

int64_t run(CPU *cpu) {
    static void *dispatch[] = {
        [NOP] = &&OP_NOP,
        [ADD] = &&OP_ADD,
        [SUB] = &&OP_SUB,
        [MUL] = &&OP_MUL,
        [DIV] = &&OP_DIV,
        [INC] = &&OP_INC,
        [DEC] = &&OP_DEC,
        [JMP] = &&OP_JMP,
        [JE] = &&OP_JE,
        [JNE] = &&OP_JNE,
        [JL] = &&OP_JL,
        [JG] = &&OP_JG,
        [JLE] = &&OP_JLE,
        [JGE] = &&OP_JGE,
        [SETZ] = &&OP_SETZ,
        [MOV] = &&OP_MOV,
        [LSH] = &&OP_LSH,
        [RSH] = &&OP_RSH,
        [OR] = &&OP_OR,
        [AND] = &&OP_AND,
        [XOR] = &&OP_XOR,
        [CALL] = &&OP_CALL,
        [RET] = &&OP_RET,
        [EOP] = &&OP_EOP,
        [EOPV] = &&OP_EOPV,
        [LOOP] = &&OP_LOOP,
        [SYSCALL] = &&OP_SYSCALL,
        [PUSH] = &&OP_PUSH,
        [POP] = &&OP_POP,
        [LOAD] = &&OP_LOAD,
        [STORE] = &&OP_STORE,
        [TIME] = &&OP_TIME,
        [DEBUG] = &&OP_DEBUG
    };

    if (!cpu->isThreaded) {
        static void *dispatchImm[64] = {
            [ADD] = &&OP_ADD_IMM,
            [SUB] = &&OP_SUB_IMM,
            [MUL] = &&OP_MUL_IMM,
            [DIV] = &&OP_DIV_IMM,
            [MOV] = &&OP_MOV_IMM,
            [LSH] = &&OP_LSH_IMM,
            [RSH] = &&OP_RSH_IMM,
            [OR] = &&OP_OR_IMM,
            [AND] = &&OP_AND_IMM,
            [XOR] = &&OP_XOR_IMM,
            [CALL] = &&OP_CALL_IMM,
            [JMP] = &&OP_JMP,
            [JE] = &&OP_JE,
            [JNE] = &&OP_JNE,
            [JL] = &&OP_JL,
            [JG] = &&OP_JG,
            [JLE] = &&OP_JLE,
            [JGE] = &&OP_JGE
        };

        for (ram_addr_t i = 0; i < cpu->decodedSize; i++) {
            instr_t *ins = &cpu->decodedProgram[i];
            if ((ins->mode & ADDR_MODE_IMM) && dispatchImm[ins->opcode]) {
                ins->handler = dispatchImm[ins->opcode];
            } else {
                ins->handler = dispatch[ins->opcode];
            }
        }
        cpu->isThreaded = true;
    }

    cpu->running = true;

    register instr_t *p_ins = &cpu->decodedProgram[0];
    register int64_t *const regs = cpu->regs.registers;
    Registers *const m_regs = &cpu->regs;
    const uint8_t *ram_data = cpu->ram.data;
    uint8_t *m_ram_data = cpu->ram.data;

#define NEXT() do { \
    p_ins++; \
    goto *p_ins->handler; \
} while (0)

#define JUMP(target_addr) do { \
    p_ins = p_ins->jump_target; \
    goto *p_ins->handler; \
} while (0)

#define GET_SRC1 (p_ins->imm)
#define GET_SRC2 (p_ins->imm)
#define READ_REG(reg) (regs[(reg)])
#define WRITE_REG(reg, val) (regs[(reg)] = (val))

#define BINARY_OP(op) do { \
    regs[p_ins->dst] = regs[p_ins->src1] op regs[p_ins->src2]; \
    p_ins++; \
    goto *p_ins->handler; \
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
        p_ins++;
        goto *p_ins->handler;
    }

OP_ADD: {
        BINARY_OP(+);
    }

OP_ADD_IMM: {
        regs[p_ins->dst] = regs[p_ins->src1] + p_ins->imm;
        NEXT();
    }

OP_SUB: {
        BINARY_OP(-);
    }

OP_SUB_IMM: {
        regs[p_ins->dst] = regs[p_ins->src1] - p_ins->imm;
        NEXT();
    }

OP_MUL: {
        BINARY_OP(*);
    }

OP_MUL_IMM: {
        regs[p_ins->dst] = regs[p_ins->src1] * p_ins->imm;
        NEXT();
    }

OP_DIV: {
        const int64_t divisor = regs[p_ins->src2];
        if (divisor == 0) {
            cpu->running = false;
            cpu->returnValue = -1;
            goto FINAL;
        }

        regs[p_ins->dst] = regs[p_ins->src1] / divisor;
        NEXT();
    }

OP_DIV_IMM: {
        const int64_t divisor = p_ins->imm;
        if (divisor == 0) {
            cpu->running = false;
            cpu->returnValue = -1;
            goto FINAL;
        }

        regs[p_ins->dst] = regs[p_ins->src1] / divisor;
        NEXT();
    }

OP_INC: {
        regs[p_ins->dst]++;
        p_ins++;
        goto *p_ins->handler;
    }

OP_DEC: {
        regs[p_ins->dst]--;
        p_ins++;
        goto *p_ins->handler;
    }

OP_JMP: {
        JUMP(0);
    }

OP_JE: {
        if (regs[p_ins->src1] == 0) {
            JUMP(0);
        }
        p_ins++;
        goto *p_ins->handler;
    }

OP_JNE: {
        if (regs[p_ins->src1] != 0) {
            JUMP(0);
        }
        p_ins++;
        goto *p_ins->handler;
    }

OP_JL: {
        if (regs[p_ins->src1] < 0) {
            JUMP(0);
        }
        NEXT();
    }

OP_JG: {
        const int64_t val = READ_REG(p_ins->src1);
        if (val > 0) {
            JUMP(0);
        }
        NEXT();
    }

OP_JLE: {
        if (regs[p_ins->src1] <= 0) {
            JUMP(0);
        }
        NEXT();
    }

OP_JGE: {
        if (regs[p_ins->src1] >= 0) {
            JUMP(0);
        }
        NEXT();
    }

OP_SETZ: {
        WRITE_REG(p_ins->dst, (int64_t)m_regs->zero_flag);
        NEXT();
    }

OP_MOV: {
        regs[p_ins->dst] = regs[p_ins->src1];
        p_ins++;
        goto *p_ins->handler;
    }

OP_MOV_IMM: {
        regs[p_ins->dst] = p_ins->imm;
        p_ins++;
        goto *p_ins->handler;
    }

OP_LSH: {
        regs[p_ins->dst] = (int64_t)((uint64_t)regs[p_ins->src1] << (regs[p_ins->src2] & 63));
        NEXT();
    }

OP_LSH_IMM: {
        regs[p_ins->dst] = (int64_t)((uint64_t)regs[p_ins->src1] << (p_ins->imm & 63));
        NEXT();
    }

OP_RSH: {
        regs[p_ins->dst] = (int64_t)((uint64_t)regs[p_ins->src1] >> (regs[p_ins->src2] & 63));
        NEXT();
    }

OP_RSH_IMM: {
        regs[p_ins->dst] = (int64_t)((uint64_t)regs[p_ins->src1] >> (p_ins->imm & 63));
        NEXT();
    }

OP_OR: {
        BINARY_OP(|);
    }

OP_OR_IMM: {
        regs[p_ins->dst] = regs[p_ins->src1] | p_ins->imm;
        NEXT();
    }

OP_AND: {
        BINARY_OP(&);
    }

OP_AND_IMM: {
        regs[p_ins->dst] = regs[p_ins->src1] & p_ins->imm;
        NEXT();
    }

OP_XOR: {
        BINARY_OP(^);
    }

OP_XOR_IMM: {
        regs[p_ins->dst] = regs[p_ins->src1] ^ p_ins->imm;
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

        uint64_t target = (uint64_t)regs[p_ins->src1];
        ram_addr_t _pci = 0;
        while (_pci < cpu->decodedSize && cpu->decodedProgram[_pci].addr < target) {
            _pci++;
        }
        p_ins = &cpu->decodedProgram[_pci];
        goto *p_ins->handler;
    }

OP_CALL_IMM: {
        if (m_regs->stack_pointer < 8) {
            cpu->running = false;
            cpu->returnValue = -1;
            goto FINAL;
        }

        uint64_t ret_index = (uint64_t)(p_ins - cpu->decodedProgram + 1);

        m_regs->stack_pointer -= 8;
        *(uint64_t *)&m_ram_data[m_regs->stack_pointer] = ret_index;

        JUMP(0);
    }

OP_RET: {
        CHECK_MEM(m_regs->stack_pointer, 8);

        uint64_t ret = *(uint64_t *)&ram_data[m_regs->stack_pointer];
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
             case 2: // PRINT LONG R0
                 printf("%ld", READ_REG(0));
                 break;
             case 3: { // PRINT CHAR R0 (UTF-8)
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
             case 6: // GETCHAR -> R0
                 WRITE_REG(0, (int64_t)getchar());
                 break;
             case 8: // EXIT R0
                 cpu->running = false;
                 cpu->returnValue = READ_REG(0);
                 goto FINAL;
             case 9: // PRINT NEWLINE
                 printf("\n");
                 break;
             case 10: // PRINT HASH R0
                 printf("Hash: %lx\n", READ_REG(0));
                 break;
             case 11: // PRINT DEC (HEX) R0
                 printf("%ld (0x%lx)\n", READ_REG(0), READ_REG(0));
                 break;
             case 12: // PRINT HEX R0
                 printf("%lx\n", READ_REG(0));
                 break;
             case 13: // PRINT BINARY R0
                 {
                     int64_t val = READ_REG(0);
                     for (int i = 63; i >= 0; i--) {
                         printf("%d", (int)((val >> i) & 1));
                     }
                     printf("\n");
                 }
                 break;
             case 15: { // MEMCPY(dest=R1, src=R2, len=R3)
                 uint64_t dest = READ_REG(1);
                 uint64_t src = READ_REG(2);
                 int64_t len = READ_REG(3);
                 if (len < 0) { cpu->running = false; cpu->returnValue = -1; goto FINAL; }
                 CHECK_MEM(dest, (size_t)len);
                 CHECK_MEM(src, (size_t)len);
                 memcpy(&m_ram_data[dest], &ram_data[src], (size_t)len);
                 break;
             }
             case 16: { // MEMSET(addr=R1, val=R2, len=R3)
                 uint64_t addr = READ_REG(1);
                 int64_t len = READ_REG(3);
                 if (len < 0) { cpu->running = false; cpu->returnValue = -1; goto FINAL; }
                 CHECK_MEM(addr, (size_t)len);
                 memset(&m_ram_data[addr], (int)READ_REG(2), (size_t)len);
                 break;
             }
             case 17: { // NANOSLEEP(us=R0)
                 int64_t us = READ_REG(0);
                 if (us < 0) { cpu->running = false; cpu->returnValue = -1; goto FINAL; }
                 struct timespec req;
                 req.tv_sec = us / 1000000;
                 req.tv_nsec = (us % 1000000) * 1000;
                 nanosleep(&req, NULL);
                 break;
             }
             case 20: { // OPEN(path_addr=R1, flags=R2) -> R0 (fd)
                 uint64_t path_addr = READ_REG(1);
                 // Simple check for path length or valid RAM
                 CHECK_MEM(path_addr, 1);
                 const char *path = (const char *)&ram_data[path_addr];
                 // Note: path must be null-terminated in RAM
                 int fd = open(path, (int)READ_REG(2), 0644);
                 WRITE_REG(0, (int64_t)fd);
                 break;
             }
             case 21: { // READ(fd=R1, buf_addr=R2, len=R3) -> R0 (bytes)
                 int fd = (int)READ_REG(1);
                 uint64_t buf_addr = READ_REG(2);
                 int64_t len = READ_REG(3);
                 if (len < 0) { cpu->running = false; cpu->returnValue = -1; goto FINAL; }
                 CHECK_MEM(buf_addr, (size_t)len);
                 ssize_t bytes = read(fd, &m_ram_data[buf_addr], (size_t)len);
                 WRITE_REG(0, (int64_t)bytes);
                 break;
             }
             case 22: { // WRITE(fd=R1, buf_addr=R2, len=R3) -> R0 (bytes)
                 int fd = (int)READ_REG(1);
                 uint64_t buf_addr = READ_REG(2);
                 int64_t len = READ_REG(3);
                 if (len < 0) { cpu->running = false; cpu->returnValue = -1; goto FINAL; }
                 CHECK_MEM(buf_addr, (size_t)len);
                 ssize_t bytes = write(fd, &ram_data[buf_addr], (size_t)len);
                 WRITE_REG(0, (int64_t)bytes);
                 break;
             }
             case 23: { // CLOSE(fd=R1) -> R0
                 int fd = (int)READ_REG(1);
                 WRITE_REG(0, (int64_t)close(fd));
                 break;
             }
             case 24: { // LSEEK(fd=R1, offset=R2, whence=R3) -> R0
                 int fd = (int)READ_REG(1);
                 off_t offset = (off_t)READ_REG(2);
                 int whence = (int)READ_REG(3);
                 WRITE_REG(0, (int64_t)lseek(fd, offset, whence));
                 break;
             }
             case 30: { // BRK(new_brk=R1) -> R0 (current brk)
                 // This is a simplified memory expansion
                 uint64_t new_size = READ_REG(1);
                 if (new_size > cpu->ram.size) {
                     uint8_t *new_data = realloc(cpu->ram.data, new_size);
                     if (new_data) {
                         memset(new_data + cpu->ram.size, 0, new_size - cpu->ram.size);
                         cpu->ram.data = new_data;
                         cpu->ram.size = new_size;
                         // Update pointers that might have changed
                         ram_data = cpu->ram.data;
                         m_ram_data = cpu->ram.data;
                     }
                 }
                 WRITE_REG(0, (int64_t)cpu->ram.size);
                 break;
             }
             case 40: { // GETENV(name_addr=R1) -> R0 (val_addr in RAM or 0)
                 uint64_t name_addr = READ_REG(1);
                 uint64_t buf_addr = READ_REG(2);
                 int64_t buf_len = READ_REG(3);
                 CHECK_MEM(name_addr, 1);
                 CHECK_MEM(buf_addr, (size_t)buf_len);
                 char *val = getenv((const char *)&ram_data[name_addr]);
                 if (val) {
                     strncpy((char *)&m_ram_data[buf_addr], val, (size_t)buf_len);
                     WRITE_REG(0, 1);
                 } else {
                     WRITE_REG(0, 0);
                 }
                 break;
             }
             default:
                 cpu->running = false;
                 cpu->returnValue = -1;
                 goto FINAL;
         }
         NEXT();
     }

OP_PUSH: {
        if (m_regs->stack_pointer < 8) {
            cpu->running = false;
            cpu->returnValue = -1;
            goto FINAL;
        }
        m_regs->stack_pointer -= 8;
        *(int64_t *) &m_ram_data[m_regs->stack_pointer] = READ_REG(p_ins->dst);
        NEXT();
    }

OP_POP: {
        if (m_regs->stack_pointer + 8 > cpu->ram.size) {
            cpu->running = false;
            cpu->returnValue = -1;
            goto FINAL;
        }
        WRITE_REG(p_ins->dst, *(int64_t*)&ram_data[m_regs->stack_pointer]);
        m_regs->stack_pointer += 8;
        NEXT();
    }

OP_LOAD: {
        uint64_t addr = (uint64_t)READ_REG(p_ins->src1);
        CHECK_MEM(addr, 8);
        int64_t tmp;
        memcpy(&tmp, &cpu->ram.data[addr], 8);
        WRITE_REG(p_ins->dst, tmp);
        NEXT();
    }

OP_STORE: {
        uint64_t addr = (uint64_t)READ_REG(p_ins->dst);
        int64_t val = READ_REG(p_ins->src1);
        CHECK_MEM(addr, 8);
        *(int64_t *)&m_ram_data[addr] = val;
        NEXT();
    }

OP_TIME: {
        WRITE_REG(p_ins->dst, (int64_t)time(NULL));
        NEXT();
    }

OP_DEBUG: {
        cpu_dump_registers(cpu);
        printf("RAM Size: %lu bytes\n", cpu->ram.size);
        printf("Stack Pointer: %lu\n", m_regs->stack_pointer);
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
                instr->imm |= (int64_t)0xFFFFFFFF00000000;
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
