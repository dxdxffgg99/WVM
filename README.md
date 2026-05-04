# WVM – A Lightweight Virtual Machine

WVM is a small, self‑contained virtual machine written in C.  
It assembles a simple assembly language into bytecode and executes it on a simulated CPU with its own memory model.

## Features

- **Assembler**: `src/asm/assembler.c` converts text assembly to binary instructions.
- **Instruction set**: 51 opcodes (e.g., `ADD`, `MUL`, `JMP`, `CALL`, `EOPV`) defined in `src/bytecode/opcode.h`.
- **CPU emulation**: Implements registers, flags, stack, and memory access with bounds checking.
- **Jump/fusion optimization**: Detects common instruction patterns (e.g., `INC + CMP + JL`) and replaces them with fused opcodes for faster execution.
- **Portable build**: Uses CMake (`CMakeLists.txt`) and compiles on Linux/macOS/Windows.

## Building
```
bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```
The executable is generated as `wvm`.

this project is tested on Linux 7.0.3-arch1-2, Ubuntu-latest.
computed goto is not supported on MSVC so this program may not work on Windows.

## Running a Program

A simple example program is embedded in `main.c`.  
It demonstrates the assembly syntax and prints execution time.
```
bash
./wvm
```
Output will include:

- Assembly compilation time.
- VM run time.
- Return value (`RV`) of the last executed instruction.

## Assembly Syntax Overview

| Mnemonic | Description |
|----------|-------------|
| `mov $imm, %reg` | Move immediate to register |
| `add $imm, %reg` | Add immediate to register |
| `mul %reg1, %reg2` | Multiply two registers |
| `jmp %label` | Unconditional jump |
| `je %label` / `jne`, `jl`, etc. | Conditional jumps based on flags |
| `call %label` | Function call (push return address) |
| `ret` | Return from function |
| `eopv %reg` | End program with return value in register |

Labels are numeric addresses or symbolic names resolved by the assembler.

## Project Structure
```

WVM/
├─ src/
│  ├─ asm/          # Assembler implementation
│  ├─ bytecode/     # Opcode definitions
│  ├─ cpu/          # CPU execution engine
│  └─ wvm.h         # Public API header
├─ CMakeLists.txt   # Build configuration
└─ main.c           # Demo program and entry point
```
## Extending WVM

- **Add new opcodes**: Define in `opcode.h`, implement handlers in `cpu.c`.
- **Improve assembler**: Support macros, more addressing modes.
- **Performance**: Profile and optimize hot paths or add JIT.

## Benchmark

i9 14900k @ 6.00GHz

```assembly
mov $0, %r0
mov $0, %r1
mov $1000000000, %r2
loop:
    add $1, %r0
    mul %r0, $2
    sub %r0, $1
    and %r0, $0xFF
    add $1, %r1
    cmp %r1, %r2
    jl loop
eopv %r0
```

```shell
Assemble start
Assemble finished
VM start
VM finished
Assembly time: 0.000024 s
VM Run time:   6.940064 s
RV: 1000000000
```

1.01 GIPS on this benchmark. (6 cpu cycles/instr.)

## License

WVM is released under the MIT license. See `LICENSE` for details.

--- 

For any questions or contributions, feel free to open an issue or pull request on the repository.
