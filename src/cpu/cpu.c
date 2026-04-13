#include "cpu.h"
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static inline int64_t imm64(const RAM* ram, const uint64_t pc) { return mem_read64(ram, pc); }

int cpu_init(CPU* cpu, uint64_t ram_size) {
    if (!(cpu && ram_size)) { return 1; }

    cpu->ram.data = (uint8_t*)malloc(ram_size);
    if (cpu->ram.data == NULL) { return 2; }

    cpu->ram.size = ram_size;
    memset(cpu->ram.data, 0, ram_size);

    memset(&cpu->registors, 0, sizeof(Registers));
    cpu->registors.stack_pointer = ram_size;

    cpu->pc = 0;
    cpu->running = 0;

    return 0;
}

void cpu_free(CPU* cpu) {
    if (cpu && cpu->ram.data) {
        free(cpu->ram.data);
        cpu->ram.data = NULL;
        cpu->ram.size = 0;
    }
}

CPU_fn run(CPU* cpu) {
    cpu->running = 1;

    while (cpu->running) {

        uint64_t pc      = cpu->pc;
        uint8_t  opcode  = mem_read8(&cpu->ram, pc);
        uint64_t next_pc = pc + 1;
        uint64_t rng = (uint64_t)time(NULL);

        switch (opcode) {

            case NOP: {
                break;
            }

            case ADD: {
                uint8_t dst1 = mem_read8(&cpu->ram, next_pc++);
                uint8_t src1 = mem_read8(&cpu->ram, next_pc++);
                uint8_t src2 = mem_read8(&cpu->ram, next_pc++);

                reg_write(&cpu->registors, dst1,
                    reg_read(&cpu->registors, src1) +
                    reg_read(&cpu->registors, src2));
                break;
            }

            case ADDI: {
                uint8_t dst1 = mem_read8(&cpu->ram, next_pc++);
                uint8_t src1 = mem_read8(&cpu->ram, next_pc++);

                int64_t imm1 = imm64(&cpu->ram, next_pc);
                next_pc += 8;

                reg_write(&cpu->registors, dst1,
                    reg_read(&cpu->registors, src1) + imm1);
                break;
            }

            case SUB: {
                uint8_t dst1 = mem_read8(&cpu->ram, next_pc++);
                uint8_t src1 = mem_read8(&cpu->ram, next_pc++);
                uint8_t src2 = mem_read8(&cpu->ram, next_pc++);

                reg_write(&cpu->registors, dst1,
                    reg_read(&cpu->registors, src1) -
                    reg_read(&cpu->registors, src2));
                break;
            }

            case SUBI: {
                uint8_t dst1 = mem_read8(&cpu->ram, next_pc++);
                uint8_t src1 = mem_read8(&cpu->ram, next_pc++);

                int64_t imm1 = imm64(&cpu->ram, next_pc);
                next_pc += 8;

                reg_write(&cpu->registors, dst1,
                    reg_read(&cpu->registors, src1) - imm1);
                break;
            }

            case MUL: {
                uint8_t dst1 = mem_read8(&cpu->ram, next_pc++);
                uint8_t src1 = mem_read8(&cpu->ram, next_pc++);
                uint8_t src2 = mem_read8(&cpu->ram, next_pc++);

                reg_write(&cpu->registors, dst1,
                    reg_read(&cpu->registors, src1) *
                    reg_read(&cpu->registors, src2));
                break;
            }

            case MULI: {
                uint8_t dst1 = mem_read8(&cpu->ram, next_pc++);
                uint8_t src1 = mem_read8(&cpu->ram, next_pc++);

                int64_t imm1 = imm64(&cpu->ram, next_pc);
                next_pc += 8;

                reg_write(&cpu->registors, dst1,
                    reg_read(&cpu->registors, src1) * imm1);
                break;
            }

            case DIV: {
                uint8_t dst1 = mem_read8(&cpu->ram, next_pc++);
                uint8_t src1 = mem_read8(&cpu->ram, next_pc++);
                uint8_t src2 = mem_read8(&cpu->ram, next_pc++);

                int64_t v_src2 = reg_read(&cpu->registors, src2);
                if (v_src2== 0) { return CPU_ERR_DIV_ZERO; }

                reg_write(&cpu->registors, dst1,
                    reg_read(&cpu->registors, src1) / v_src2);
                break;
            }

            case DIVI: {
                uint8_t dst1 = mem_read8(&cpu->ram, next_pc++);
                uint8_t src1 = mem_read8(&cpu->ram, next_pc++);

                int64_t imm1 = imm64(&cpu->ram, next_pc);
                next_pc += 8;

                if (imm1 == 0) { return CPU_ERR_DIV_ZERO; }

                reg_write(&cpu->registors, dst1,
                    reg_read(&cpu->registors, src1) / imm1);
                break;
            }

            case OR: {
                uint8_t dst1 = mem_read8(&cpu->ram, next_pc++);
                uint8_t src1 = mem_read8(&cpu->ram, next_pc++);
                uint8_t src2 = mem_read8(&cpu->ram, next_pc++);

                reg_write(&cpu->registors, dst1,
                    reg_read(&cpu->registors, src1) |
                    reg_read(&cpu->registors, src2));
                break;
            }

            case ORI: {
                uint8_t dst1 = mem_read8(&cpu->ram, next_pc++);
                uint8_t src1 = mem_read8(&cpu->ram, next_pc++);

                int64_t imm1 = imm64(&cpu->ram, next_pc);
                next_pc += 8;

                reg_write(&cpu->registors, dst1,
                    reg_read(&cpu->registors, src1) | imm1);
                break;
            }

            case AND: {
                uint8_t dst1 = mem_read8(&cpu->ram, next_pc++);
                uint8_t src1 = mem_read8(&cpu->ram, next_pc++);
                uint8_t src2 = mem_read8(&cpu->ram, next_pc++);

                reg_write(&cpu->registors, dst1,
                    reg_read(&cpu->registors, src1) &
                    reg_read(&cpu->registors, src2));
                break;
            }

            case ANDI: {
                uint8_t dst1 = mem_read8(&cpu->ram, next_pc++);
                uint8_t src1 = mem_read8(&cpu->ram, next_pc++);

                int64_t imm1 = imm64(&cpu->ram, next_pc);
                next_pc += 8;

                reg_write(&cpu->registors, dst1,
                    reg_read(&cpu->registors, src1) & imm1);
                break;
            }

            case XOR: {
                uint8_t dst1 = mem_read8(&cpu->ram, next_pc++);
                uint8_t src1 = mem_read8(&cpu->ram, next_pc++);
                uint8_t src2 = mem_read8(&cpu->ram, next_pc++);

                reg_write(&cpu->registors, dst1,
                    reg_read(&cpu->registors, src1) ^
                    reg_read(&cpu->registors, src2));
                break;
            }

            case XORI: {
                uint8_t dst1 = mem_read8(&cpu->ram, next_pc++);
                uint8_t src1 = mem_read8(&cpu->ram, next_pc++);

                int64_t imm1 = imm64(&cpu->ram, next_pc);
                next_pc += 8;

                reg_write(&cpu->registors, dst1,
                    reg_read(&cpu->registors, src1) ^ imm1);
                break;
            }

            case JMP: {
                uint8_t r = mem_read8(&cpu->ram, next_pc++);
                cpu->pc = reg_read(&cpu->registors, r);
                continue;
            }

            case JMPI: {
                cpu->pc = imm64(&cpu->ram, next_pc);
                continue;
            }

            case CMP: {
                uint8_t src1 = mem_read8(&cpu->ram, next_pc++);
                uint8_t src2 = mem_read8(&cpu->ram, next_pc++);

                int64_t v_src1 = (int64_t)reg_read(&cpu->registors, src1);
                int64_t v_src2 = (int64_t)reg_read(&cpu->registors, src2);

                int64_t res = v_src1 - v_src2;

                cpu->registors.zero_flag = (res == 0);
                cpu->registors.sign_flag = (res < 0);
                break;
            }

            case CMPI: {
                uint8_t src1 = mem_read8(&cpu->ram, next_pc++);

                int64_t imm = imm64(&cpu->ram, next_pc);
                next_pc += 8;

                int64_t v_src1 = (int64_t)reg_read(&cpu->registors, src1);
                int64_t res = v_src1 - imm;

                cpu->registors.zero_flag = (res == 0);
                cpu->registors.sign_flag = (res < 0);
                cpu->registors.carry_flag = (v_src1 < imm);

                break;
            }


            case JE: {
                int64_t addr = imm64(&cpu->ram, next_pc);
                next_pc += 8;

                if (cpu->registors.zero_flag) {
                    cpu->pc = addr;
                    continue;
                }
                break;
            }

            case JNE: {
                int64_t addr = imm64(&cpu->ram, next_pc);
                next_pc += 8;

                if (!cpu->registors.zero_flag) {
                    cpu->pc = addr;
                    continue;
                }
                break;
            }

            case JL: {
                int64_t addr = imm64(&cpu->ram, next_pc);
                next_pc += 8;

                if (cpu->registors.sign_flag) {
                    cpu->pc = addr;
                    continue;
                }
                break;
            }

            case JG: {
                int64_t addr = imm64(&cpu->ram, next_pc);
                next_pc += 8;

                if (!cpu->registors.zero_flag && !cpu->registors.sign_flag) {
                    cpu->pc = addr;
                    continue;
                }
                break;
            }

            case JLE: {
                int64_t addr = imm64(&cpu->ram, next_pc);
                next_pc += 8;

                if (cpu->registors.zero_flag || cpu->registors.sign_flag) {
                    cpu->pc = addr;
                    continue;
                }
                break;
            }

            case JGE: {
                int64_t addr = imm64(&cpu->ram, next_pc);
                next_pc += 8;

                if (!cpu->registors.sign_flag) {
                    cpu->pc = addr;
                    continue;
                }
                break;
            }

            case LOAD: {
                uint8_t dst1 = mem_read8(&cpu->ram, next_pc++);
                uint8_t base = mem_read8(&cpu->ram, next_pc++);

                int64_t off = imm64(&cpu->ram, next_pc);
                next_pc += 8;

                uint64_t addr =
                    (uint64_t)reg_read(&cpu->registors, base) + off;

                reg_write(&cpu->registors, dst1,
                    mem_read8(&cpu->ram, addr));
                break;
            }

            case STORE: {
                uint8_t base = mem_read8(&cpu->ram, next_pc++);
                uint8_t src = mem_read8(&cpu->ram, next_pc++);

                int64_t off = imm64(&cpu->ram, next_pc);
                next_pc += 8;

                uint64_t addr =
                    (uint64_t)reg_read(&cpu->registors, base) + off;

                mem_write8(&cpu->ram, addr,
                    reg_read(&cpu->registors, src));
                break;
            }

            case TIME: {
                uint8_t dst1 = mem_read8(&cpu->ram, next_pc++);

                struct timespec ts;
                clock_gettime(CLOCK_REALTIME, &ts);

                int64_t ms =
                    (int64_t)ts.tv_sec * 1000 +
                    (int64_t)ts.tv_nsec / 1000000;

                reg_write(&cpu->registors, dst1, ms);
                break;
            }

            case RAND: {
                uint8_t dst1 = mem_read8(&cpu->ram, next_pc++);
                uint8_t src1 = mem_read8(&cpu->ram, next_pc++);

                uint64_t x = (uint64_t)reg_read(&cpu->registors, src1);

                x ^= x << 13;
                x ^= x >> 7;
                x ^= x << 17;

                x ^= x;
                rng = x;

                reg_write(&cpu->registors, dst1, (int64_t)rng);
                break;
            }

            case PUSH: {
                uint8_t src1 = mem_read8(&cpu->ram, next_pc++);

                int64_t v_src1 = reg_read(&cpu->registors, src1);

                cpu->registors.stack_pointer -= 8;
                mem_write64(&cpu->ram, cpu->registors.stack_pointer, v_src1);

                break;
            }

            case PUSHI: {
                int64_t imm1 = imm64(&cpu->ram, next_pc);
                next_pc += 8;

                cpu->registors.stack_pointer -= 8;
                mem_write64(&cpu->ram, cpu->registors.stack_pointer, imm1);

                break;
            }

            case POP: {
                uint8_t dst1 = mem_read8(&cpu->ram, next_pc++);

                int64_t value = mem_read64(&cpu->ram, cpu->registors.stack_pointer);
                cpu->registors.stack_pointer += 8;

                reg_write(&cpu->registors, dst1, value);

                break;
            }

            case CALL: {
                int64_t target = imm64(&cpu->ram, next_pc);
                next_pc += 8;

                cpu->registors.stack_pointer -= 8;
                mem_write64(&cpu->ram, cpu->registors.stack_pointer, (int64_t)next_pc);

                next_pc = target;
                break;
            }

            case RET: {
                int64_t ret_addr = mem_read64(&cpu->ram, cpu->registors.stack_pointer);
                cpu->registors.stack_pointer += 8;

                next_pc = ret_addr;
                break;
            }

            case EOP: {
                cpu->running = 0;
                return CPU_OK;
            }

            case EOPV: {
                return reg_read(&cpu->registors,mem_read8(&cpu->ram, next_pc));
            }

            case EOPVI: {
                return imm64(&cpu->ram, next_pc);
            }

            case DEBUG: {
                printf("=======[debug]=======\n");

                printf("RAM : \n");
                for (int i = 0; i < cpu->ram.size; i++) {
                    printf("%02x ", mem_read8(&cpu->ram, i));
                    if ( !((i + 1) % 32) ) { printf("\n"); }
                } printf("\n\n");

                printf("Registers : \n");
                for (int i = 0; i < 64; i++) {
                    printf("%ld ", reg_read(&cpu->registors, i));
                    if ( !((i + 1) % 8) ) printf("\n");
                } printf("\n");

                printf("=====================\n");
            }

            default: {
                return CPU_ERR_UNKNOWN_COMMAND;
            }
        }

        cpu->pc = next_pc;
    }

    return CPU_OK;
}
