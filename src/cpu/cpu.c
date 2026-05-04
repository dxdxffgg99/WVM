#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int64_t run(CPU* cpu) {
    static void* dispatch[] = {
        &&OP_NOP, &&OP_ADD, &&OP_SUB, &&OP_MUL, &&OP_DIV,
        &&OP_INC, &&OP_DEC, &&OP_JMP, &&OP_CMP, &&OP_JE,
        &&OP_JNE, &&OP_JL, &&OP_JG, &&OP_JLE, &&OP_JGE,
        &&OP_SETZ, &&OP_MOV, &&OP_LSH, &&OP_RSH, &&OP_LOAD,
        &&OP_STORE, &&OP_OR, &&OP_AND, &&OP_XOR, &&OP_TIME,
        &&OP_PUSH, &&OP_POP, &&OP_CALL, &&OP_RET, &&OP_EOP,
        &&OP_EOPV, &&OP_DEBUG, &&OP_LOOP, &&OP_ADD_LOOP_IMM,
        &&OP_INC_LOOP, &&OP_DEC_LOOP, &&OP_CMP_JE_IMM, &&OP_CMP_JNE_IMM,
        &&OP_CMP_JL_IMM, &&OP_CMP_JG_IMM, &&OP_CMP_JLE_IMM, &&OP_CMP_JGE_IMM,
        &&OP_MADD, &&OP_CMP_JE_REG, &&OP_CMP_JNE_REG, &&OP_CMP_JL_REG, &&OP_CMP_JG_REG,
        &&OP_CMP_JLE_REG, &&OP_CMP_JGE_REG, &&OP_INC_CMP_JL_IMM, &&OP_MOV_STORE_IMM
    };

    if (!cpu->is_threaded) {
        for (ram_addr_t i = 0; i < cpu->decoded_size; i++) {
            cpu->decoded_program[i].handler = dispatch[cpu->decoded_program[i].opcode];
        }
        cpu->is_threaded = true;
    }

    cpu->running = true;

    instr_t* p_ins = &cpu->decoded_program[0];
    const Registers* regs = &cpu->regs;
    Registers* m_regs = &cpu->regs;
    const uint8_t* ram_data = cpu->ram.data;
    uint8_t* m_ram_data = cpu->ram.data;

#define NEXT() do { \
    p_ins++; \
    goto *p_ins->handler; \
} while (0)

#define JUMP(target_addr) do { \
    if (p_ins->jump_target) { \
        p_ins = p_ins->jump_target; \
    } else { \
        uint64_t _addr = (target_addr); \
        ram_addr_t _pci = 0; \
        while (_pci < cpu->decoded_size && cpu->decoded_program[_pci].addr < _addr) { \
            _pci++; \
        } \
        p_ins = &cpu->decoded_program[_pci]; \
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
        cpu->rv = -1; \
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
            cpu->rv = -1;
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
        cpu->regs.carry_flag = ((uint64_t)a < (uint64_t)b);

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
        *(int64_t*)&m_ram_data[GET_SRC2] = READ_REG(p_ins->dst);
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
            cpu->rv = -1;
            goto FINAL;
        }
        m_regs->stack_pointer -= 8;
        *(int64_t*)&m_ram_data[regs->stack_pointer] = GET_SRC1;
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
            cpu->rv = -1;
            goto FINAL;
        }
        m_regs->stack_pointer -= 8;
        *(int64_t*)&m_ram_data[regs->stack_pointer] = (int64_t)p_ins->addr + p_ins->size;
        JUMP(GET_SRC1);
    }

    OP_RET: {
        CHECK_MEM(regs->stack_pointer, 8);
        const uint64_t target = *(uint64_t*)&ram_data[regs->stack_pointer];
        m_regs->stack_pointer += 8;
        JUMP(target);
    }

    OP_EOP: {
        cpu->running = false;
        cpu->rv = 0;
        goto FINAL;
    }

    OP_EOPV: {
        cpu->running = false;
        cpu->rv = READ_REG(p_ins->src1);
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

    OP_ADD_LOOP_IMM: {
        WRITE_REG(p_ins->dst, READ_REG(p_ins->dst) + p_ins->imm);
        const int64_t val = READ_REG(p_ins->src2) - 1;
        WRITE_REG(p_ins->src2, val);
        if (val > 0) {
            p_ins = p_ins->jump_target;
            goto *p_ins->handler;
        }
        p_ins += 2;
        goto *p_ins->handler;
    }

    OP_INC_LOOP: {
        WRITE_REG(p_ins->dst, READ_REG(p_ins->dst) + 1);
        const int64_t val = READ_REG(p_ins->src2) - 1;
        WRITE_REG(p_ins->src2, val);
        if (val > 0) {
            p_ins = p_ins->jump_target;
            goto *p_ins->handler;
        }
        p_ins += 2;
        goto *p_ins->handler;
    }

    OP_DEC_LOOP: {
        WRITE_REG(p_ins->dst, READ_REG(p_ins->dst) - 1);
        const int64_t val = READ_REG(p_ins->src2) - 1;
        WRITE_REG(p_ins->src2, val);
        if (val > 0) {
            p_ins = p_ins->jump_target;
            goto *p_ins->handler;
        }
        p_ins += 2;
        goto *p_ins->handler;
    }

    OP_CMP_JE_IMM: {
        if (READ_REG(p_ins->src1) == p_ins->imm) {
            p_ins = p_ins->jump_target;
            goto *p_ins->handler;
        }
        p_ins += 2;
        goto *p_ins->handler;
    }

    OP_CMP_JNE_IMM: {
        if (READ_REG(p_ins->src1) != p_ins->imm) {
            p_ins = p_ins->jump_target;
            goto *p_ins->handler;
        }
        p_ins += 2;
        goto *p_ins->handler;
    }

    OP_CMP_JL_IMM: {
        if (READ_REG(p_ins->src1) < p_ins->imm) {
            p_ins = p_ins->jump_target;
            goto *p_ins->handler;
        }
        p_ins += 2;
        goto *p_ins->handler;
    }

    OP_CMP_JG_IMM: {
        if (READ_REG(p_ins->src1) > p_ins->imm) {
            p_ins = p_ins->jump_target;
            goto *p_ins->handler;
        }
        p_ins += 2;
        goto *p_ins->handler;
    }

    OP_CMP_JLE_IMM: {
        if (READ_REG(p_ins->src1) <= p_ins->imm) {
            p_ins = p_ins->jump_target;
            goto *p_ins->handler;
        }
        p_ins += 2;
        goto *p_ins->handler;
    }

    OP_CMP_JGE_IMM: {
        if (READ_REG(p_ins->src1) >= p_ins->imm) {
            p_ins = p_ins->jump_target;
            goto *p_ins->handler;
        }
        p_ins += 2;
        goto *p_ins->handler;
    }

    OP_MADD: {
        WRITE_REG(p_ins->dst, READ_REG(p_ins->dst) * READ_REG(p_ins->src1) + READ_REG(p_ins->src2));
        NEXT();
    }

    OP_CMP_JE_REG: {
        if (READ_REG(p_ins->src1) == READ_REG(p_ins->src2)) {
            p_ins = p_ins->jump_target;
            goto *p_ins->handler;
        }
        p_ins += 2;
        goto *p_ins->handler;
    }

    OP_CMP_JNE_REG: {
        if (READ_REG(p_ins->src1) != READ_REG(p_ins->src2)) {
            p_ins = p_ins->jump_target;
            goto *p_ins->handler;
        }
        p_ins += 2;
        goto *p_ins->handler;
    }

    OP_CMP_JL_REG: {
        if (READ_REG(p_ins->src1) < READ_REG(p_ins->src2)) {
            p_ins = p_ins->jump_target;
            goto *p_ins->handler;
        }
        p_ins += 2;
        goto *p_ins->handler;
    }

    OP_CMP_JG_REG: {
        if (READ_REG(p_ins->src1) > READ_REG(p_ins->src2)) {
            p_ins = p_ins->jump_target;
            goto *p_ins->handler;
        }
        p_ins += 2;
        goto *p_ins->handler;
    }

    OP_CMP_JLE_REG: {
        if (READ_REG(p_ins->src1) <= READ_REG(p_ins->src2)) {
            p_ins = p_ins->jump_target;
            goto *p_ins->handler;
        }
        p_ins += 2;
        goto *p_ins->handler;
    }

    OP_CMP_JGE_REG: {
        if (READ_REG(p_ins->src1) >= READ_REG(p_ins->src2)) {
            p_ins = p_ins->jump_target;
            goto *p_ins->handler;
        }
        p_ins += 2;
        goto *p_ins->handler;
    }

    OP_INC_CMP_JL_IMM: {
        const int64_t val = READ_REG(p_ins->dst) + 1;
        WRITE_REG(p_ins->dst, val);
        if (val < p_ins->imm) {
            p_ins = p_ins->jump_target;
            goto *p_ins->handler;
        }
        p_ins += 3;
        goto *p_ins->handler;
    }

    OP_MOV_STORE_IMM: {
        const int64_t val = p_ins->imm;
        WRITE_REG(p_ins->dst, val);
        const uint64_t addr = READ_REG(p_ins->src2);
        CHECK_MEM(addr, 8);
        *(int64_t*)&m_ram_data[addr] = val;
        p_ins += 2;
        goto *p_ins->handler;
    }

FINAL:
#undef NEXT
#undef JUMP

    return cpu->rv;
}

bool instr_decode(const uint8_t* buffer, size_t limit, instr_t* instr) {
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
        if (limit < (size_t)(INSTR_MIN_SIZE + imm_size)) return false;
        for (int i = 0; i < imm_size; i++) {
            instr->imm |= ((imm_t)buffer[5 + i]) << (8 * i);
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

int cpu_init(CPU* cpu, const ram_addr_t ram_size) {
    if (!(cpu && ram_size)) {
        return 1;
    }

    cpu->ram.data = (uint8_t*)malloc(ram_size);
    if (!cpu->ram.data) {
        return 1;
    }

    cpu->ram.size = ram_size;
    memset(cpu->ram.data, 0, ram_size);

    memset(&cpu->regs, 0, sizeof(Registers));
    cpu->regs.stack_pointer = ram_size;

    cpu->pc = 0;
    cpu->decoded_program = NULL;
    cpu->decoded_size = 0;
    cpu->running = false;
    cpu->is_threaded = false;
    cpu->rv = 0;

    return 0;
}

void cpu_free(CPU* cpu) {
    if (cpu) {
        if (cpu->ram.data) {
            free(cpu->ram.data);
            cpu->ram.data = NULL;
            cpu->ram.size = 0;
        }
        if (cpu->decoded_program) {
            free(cpu->decoded_program);
            cpu->decoded_program = NULL;
            cpu->decoded_size = 0;
        }
    }
}

void load_program(CPU* cpu, const uint8_t* code, const ram_addr_t size) {
    if (size > cpu->ram.size) {
        return;
    }
    memcpy(cpu->ram.data, code, size);
    cpu->pc = 0;
    cpu->is_threaded = false;

    if (cpu->decoded_program) {
        free(cpu->decoded_program);
        cpu->decoded_program = NULL;
    }

    cpu->decoded_program = (instr_t*)malloc(sizeof(instr_t) * (size / 5 + 1));
    cpu->decoded_size = 0;

    ram_addr_t offset = 0;
    while (offset < size) {
        instr_t ins;
        memset(&ins, 0, sizeof(instr_t));
        if (!instr_decode(cpu->ram.data + offset, size - offset, &ins)) {
            break;
        }
        ins.addr = offset;
        ins.jump_pci = -1;
        cpu->decoded_program[cpu->decoded_size++] = ins;
        offset += ins.size;
    }

    for (ram_addr_t i = 0; i < cpu->decoded_size; i++) {
        instr_t* ins = &cpu->decoded_program[i];
        switch (ins->opcode) {
            case JMP: case JE: case JNE: case JL: case JG: case JLE: case JGE: case CALL: case LOOP:
                if (ins->mode & ADDR_MODE_IMM) {
                    uint64_t target = (uint64_t)ins->imm;
                    for (ram_addr_t j = 0; j < cpu->decoded_size; j++) {
                        if (cpu->decoded_program[j].addr == target) {
                            ins->jump_pci = (int32_t)j;
                            ins->jump_target = &cpu->decoded_program[j];
                            break;
                        }
                    }
                }
                break;
            default:
                break;
        }
    }

    for (ram_addr_t i = 0; i + 1 < cpu->decoded_size; i++) {
        instr_t* ins1 = &cpu->decoded_program[i];
        instr_t* ins2 = &cpu->decoded_program[i+1];

        if (ins1->opcode == NOP) continue;

        if (i + 2 < cpu->decoded_size) {
            instr_t* ins3 = &cpu->decoded_program[i+2];
            if (ins1->opcode == INC && ins2->opcode == CMP && (ins2->mode & ADDR_MODE_IMM) && ins3->opcode == JL) {
                if (ins1->dst == ins2->src1 && (ins3->mode & ADDR_MODE_IMM)) {
                    ins1->opcode = INC_CMP_JL_IMM;
                    ins1->imm = ins2->imm;
                    ins1->jump_pci = ins3->jump_pci;
                    ins1->jump_target = ins3->jump_target;
                    ins2->opcode = NOP;
                    ins3->opcode = NOP;
                    continue;
                }
            }
        }

        if (ins1->opcode == ADD && (ins1->mode & ADDR_MODE_IMM) && ins2->opcode == LOOP) {
            if (ins2->jump_pci == (int32_t)i) {
                ins1->opcode = ADD_LOOP_IMM;
                ins1->src2 = ins2->dst;
                ins1->jump_pci = ins2->jump_pci;
                ins1->jump_target = ins2->jump_target;
                ins2->opcode = NOP;
            }
        }

        else if (ins1->opcode == INC && ins2->opcode == LOOP) {
            if (ins2->jump_pci == (int32_t)i) {
                ins1->opcode = INC_LOOP;
                ins1->src2 = ins2->dst;
                ins1->jump_pci = ins2->jump_pci;
                ins1->jump_target = ins2->jump_target;
                ins2->opcode = NOP;
            }
        }

        else if (ins1->opcode == DEC && ins2->opcode == LOOP) {
            if (ins2->jump_pci == (int32_t)i) {
                ins1->opcode = DEC_LOOP;
                ins1->src2 = ins2->dst;
                ins1->jump_pci = ins2->jump_pci;
                ins1->jump_target = ins2->jump_target;
                ins2->opcode = NOP;
            }
        }

        else if (ins1->opcode == CMP && (ins1->mode & ADDR_MODE_IMM) && (ins2->opcode >= JE && ins2->opcode <= JGE)) {
            if (ins2->mode & ADDR_MODE_IMM) {
                ins1->opcode = CMP_JE_IMM + (ins2->opcode - JE);
                ins1->jump_pci = ins2->jump_pci;
                ins1->jump_target = ins2->jump_target;
                ins2->opcode = NOP;
            }
        }

        else if (ins1->opcode == CMP && !(ins1->mode & ADDR_MODE_IMM) && (ins2->opcode >= JE && ins2->opcode <= JGE)) {
            if (ins2->mode & ADDR_MODE_IMM) {
                ins1->opcode = CMP_JE_REG + (ins2->opcode - JE);
                ins1->jump_pci = ins2->jump_pci;
                ins1->jump_target = ins2->jump_target;
                ins2->opcode = NOP;
            }
        }

        else if (ins1->opcode == MOV && (ins1->mode & ADDR_MODE_IMM) && ins2->opcode == STORE && !(ins2->mode & ADDR_MODE_IMM)) {
            if (ins1->dst == ins2->dst) {
                ins1->opcode = MOV_STORE_IMM;
                ins1->src2 = ins2->src2;
                ins2->opcode = NOP;
            }
        }

        else if (ins1->opcode == MUL && ins2->opcode == ADD && !(ins1->mode & ADDR_MODE_IMM) && !(ins2->mode & ADDR_MODE_IMM)) {
            if (ins1->dst == ins2->dst) {
                if (ins1->src1 == ins1->dst && ins2->src1 == ins2->dst) {
                    ins1->opcode = MADD;
                    ins1->src1 = ins1->src2;
                    ins1->src2 = ins2->src2;
                    ins2->opcode = NOP;
                }
            }
        }
    }
}

void cpu_dump_registers(const CPU* cpu) {
    printf("\n==============================================================================================================================================\n");
    printf("============================================================= REGISTER =======================================================================\n");
    printf("==============================================================================================================================================\n");

    for (int base = 0; base < REG_COUNT; base += 8) {
        printf("R%03d | ", base);
        for (int i = 0; i < 8 && (base + i) < REG_COUNT; i++) {
            printf("%16ld ", reg_read(&cpu->regs, (uint64_t)(base + i)));
        }
        printf("\n");
    }

    printf("==============================================================================================================================================\n");
    printf("============================================================ DEBUG END =======================================================================\n");
    printf("==============================================================================================================================================\n");
}