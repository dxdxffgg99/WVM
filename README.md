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
The executable is generated as `wvm` (or `wvm.exe` on Windows).

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

## License

WVM is released under the MIT license. See `LICENSE` for details.

--- 

For any questions or contributions, feel free to open an issue or pull request on the repository.
