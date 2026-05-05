# WVM Assembly Syntax Guide

## 목차 / Table of Contents
- [한국어 (Korean)](#한국어)
- [English](#english)

---

## 한국어

### 개요
WVM(World Virtual Machine)은 AT&T 문법의 어셈블리를 지원합니다. 이 문서는 WVM 어셈블리의 완전한 문법 가이드입니다.

### 기본 구문

#### 레지스터 (Registers)
- **형식**: `%r0` ~ `%r255`
- **설명**: 256개의 64비트 범용 레지스터
- **예시**: `%r0`, `%r1`, `%r15`

#### 즉시값 (Immediate Values)
- **형식**: `$`로 시작하는 숫자 또는 16진수
- **설명**: 상수 값
- **예시**: `$10`, `$0x1000`, `$-100`

#### 메모리 접근 (Memory Access)
- **형식**: `[%r레지스터_번호]` 또는 직접 주소
- **설명**: 메모리에서 값을 읽거나 쓰기
- **예시**: `[%r0]`, `[%r1]`

#### 레이블 (Labels)
- **형식**: `라벨_이름:`
- **설명**: 점프 대상이 되는 위치
- **예시**: `loop:`, `start:`

### 아키텍처 정보

- **워드 크기**: 64비트 (8바이트)
- **아키텍처**: 스택 기반 및 레지스터 기반 혼합
- **주소 지정 모드**: 즉시값, 레지스터, 메모리 직접 주소

---

## 명령어 (Instructions)

### 1. 산술 연산 (Arithmetic Operations)

#### `mov` - 이동
**문법**: `mov 소스, 목표`
**설명**: 값을 목표 위치에 복사
**예시**:
```assembly
mov $100, %r0          # 100을 r0에 복사
mov %r0, %r1          # r0의 값을 r1에 복사
mov $0x1000, %r2      # 16진수 값을 r2에 복사
```

#### `add` - 덧셈
**문법**: `add 소스, 목표` 또는 `add 소스1, 소스2, 목표`
**설명**: 소스 값들을 더한 결과를 목표에 저장
**예시**:
```assembly
add $10, %r0           # r0 = r0 + 10
add %r1, %r0          # r0 = r0 + r1
add %r1, $5, %r0      # r0 = r1 + 5
```

#### `sub` - 뺄셈
**문법**: `sub 소스, 목표` 또는 `sub 소스1, 소스2, 목표`
**설명**: 소스 값들을 뺀 결과를 목표에 저장
**예시**:
```assembly
sub $5, %r0            # r0 = r0 - 5
sub %r1, %r0          # r0 = r0 - r1
```

#### `mul` - 곱셈
**문법**: `mul 소스, 목표` 또는 `mul 소스1, 소스2, 목표`
**설명**: 소스 값들을 곱한 결과를 목표에 저장
**예시**:
```assembly
mul $3, %r0            # r0 = r0 * 3
mul %r1, %r0          # r0 = r0 * r1
mul %r1, %r2, %r0     # r0 = r1 * r2
```

#### `div` - 나눗셈
**문법**: `div 소수, 목표` 또는 `div 피제수, 제수, 목표`
**설명**: 피제수를 제수로 나눈 결과를 목표에 저장
**예시**:
```assembly
div $2, %r0            # r0 = r0 / 2
div %r1, %r0          # r0 = r0 / r1
```

#### `inc` - 증가
**문법**: `inc 레지스터`
**설명**: 레지스터 값을 1 증가
**예시**:
```assembly
inc %r0                # r0 = r0 + 1
inc %r1
```

#### `dec` - 감소
**문법**: `dec 레지스터`
**설명**: 레지스터 값을 1 감소
**예시**:
```assembly
dec %r0                # r0 = r0 - 1
dec %r5
```

### 2. 비트 연산 (Bitwise Operations)

#### `and` - 비트 AND
**문법**: `and 소스, 목표` 또는 `and 소스1, 소스2, 목표`
**설명**: 비트 AND 연산
**예시**:
```assembly
and $0xFF, %r0         # r0 = r0 & 0xFF
and %r1, %r0          # r0 = r0 & r1
and %r1, $0xF0, %r0   # r0 = r1 & 0xF0
```

#### `or` - 비트 OR
**문법**: `or 소스, 목표` 또는 `or 소스1, 소스2, 목표`
**설명**: 비트 OR 연산
**예시**:
```assembly
or $0x80, %r0          # r0 = r0 | 0x80
or %r1, %r0           # r0 = r0 | r1
```

#### `xor` - 비트 XOR
**문법**: `xor 소스, 목표` 또는 `xor 소스1, 소스2, 목표`
**설명**: 비트 XOR 연산
**예시**:
```assembly
xor $0xFF, %r0         # r0 = r0 ^ 0xFF
xor %r1, %r0          # r0 = r0 ^ r1
```

#### `lsh` - 좌측 시프트 (Left Shift)
**문법**: `lsh 시프트_양, 대상`
**설명**: 비트를 좌측으로 시프트
**예시**:
```assembly
lsh $3, %r0            # r0 = r0 << 3
lsh %r1, %r0          # r0 = r0 << r1
```

#### `rsh` - 우측 시프트 (Right Shift)
**문법**: `rsh 시프트_양, 대상`
**설명**: 비트를 우측으로 시프트
**예시**:
```assembly
rsh $2, %r0            # r0 = r0 >> 2
rsh %r1, %r0          # r0 = r0 >> r1
```

### 3. 비교 및 분기 (Comparison & Branching)

#### `cmp` - 비교
**문법**: `cmp 소수, 피제수` 또는 `cmp $즉시값, 레지스터`
**설명**: 두 값을 비교하고 플래그 설정 (zero_flag, sign_flag, carry_flag)
**예시**:
```assembly
cmp %r0, $10           # r0과 10을 비교
cmp %r0, %r1          # r0과 r1을 비교
```

#### `je` - Equal 점프 (Jump if Equal)
**문법**: `je 레이블` 또는 `je 레지스터`
**설명**: zero_flag가 설정되면 점프 (a == b)
**예시**:
```assembly
cmp %r0, $10
je equal_label
```

#### `jne` - Not Equal 점프 (Jump if Not Equal)
**문법**: `jne 레이블` 또는 `jne 레지스터`
**설명**: zero_flag가 설정되지 않으면 점프 (a != b)
**예시**:
```assembly
cmp %r0, $5
jne not_equal_label
```

#### `jl` - Less Than 점프 (Jump if Less)
**문법**: `jl 레이블` 또는 `jl 레지스터`
**설명**: sign_flag가 설정되면 점프 (a < b)
**예시**:
```assembly
cmp %r0, %r1
jl less_label
```

#### `jg` - Greater Than 점프 (Jump if Greater)
**문법**: `jg 레이블` 또는 `jg 레지스터`
**설명**: sign_flag와 zero_flag 모두 설정되지 않으면 점프 (a > b)
**예시**:
```assembly
cmp %r0, %r1
jg greater_label
```

#### `jle` - Less or Equal 점프
**문법**: `jle 레이블` 또는 `jle 레지스터`
**설명**: sign_flag 또는 zero_flag가 설정되면 점프 (a <= b)
**예시**:
```assembly
cmp %r0, $100
jle less_equal_label
```

#### `jge` - Greater or Equal 점프
**문법**: `jge 레이블` 또는 `jge 레지스터`
**설명**: sign_flag가 설정되지 않으면 점프 (a >= b)
**예시**:
```assembly
cmp %r0, $50
jge greater_equal_label
```

#### `jmp` - 무조건 점프
**문법**: `jmp 레이블` 또는 `jmp 레지스터`
**설명**: 무조건적으로 지정된 위치로 점프
**예시**:
```assembly
jmp start_label
jmp %r0                # r0의 주소로 점프
```

#### `loop` - 루프
**문법**: `loop 레이블`
**설명**: 레지스터 값을 1 감소시키고 0이 아니면 점프
**예시**:
```assembly
mov $10, %r2
loop_start:
  # 루프 본문
  loop loop_start      # r2를 1 감소, 0이 아니면 loop_start로 점프
```

### 4. 메모리 연산 (Memory Operations)

#### `load` - 메모리에서 로드
**문법**: `load [주소 혹은 레지스터], 목표_레지스터`
**설명**: 메모리의 값을 읽어 레지스터에 저장 (8바이트)
**예시**:
```assembly
load [%r0], %r1        # r0이 가리키는 주소의 값을 r1에 로드
load $1000, %r2       # 주소 1000의 값을 r2에 로드
```

#### `store` - 메모리에 저장
**문법**: `store 소스_레지스터, [주소 혹은 레지스터]`
**설명**: 레지스터 값을 메모리에 저장 (8바이트)
**예시**:
```assembly
store %r0, [%r1]       # r0의 값을 r1이 가리키는 주소에 저장
store %r0, $2000      # r0의 값을 주소 2000에 저장
```

### 5. 스택 연산 (Stack Operations)

#### `push` - 스택 푸시
**문법**: `push 소수` 또는 `push $즉시값`
**설명**: 값을 스택에 푸시 (스택 포인터 감소)
**예시**:
```assembly
push %r0               # r0을 스택에 푸시
push $100              # 100을 스택에 푸시
```

#### `pop` - 스택 팝
**문법**: `pop 목표_레지스터`
**설명**: 스택에서 값을 팝하여 레지스터에 저장 (스택 포인터 증가)
**예시**:
```assembly
pop %r0                # 스택의 값을 r0에 팝
pop %r1
```

### 6. 함수 호출 (Function Calls)

#### `call` - 함수 호출
**문법**: `call 라벨` 또는 `call $주소`
**설명**: 함수 호출 (반환 주소를 스택에 푸시하고 점프)
**예시**:
```assembly
call my_function
call $1000             # 주소 1000의 함수 호출
```

#### `ret` - 함수 반환
**문법**: `ret`
**설명**: 함수에서 반환 (스택에서 반환 주소를 팝하고 점프)
**예시**:
```assembly
my_function:
  # 함수 본문
  ret                  # 호출자에게 반환
```

### 7. 유틸리티 명령어 (Utility Instructions)

#### `nop` - No Operation
**문법**: `nop`
**설명**: 아무것도 하지 않음
**예시**:
```assembly
nop
```

#### `time` - 현재 시간 얻기
**문법**: `time 목표_레지스터`
**설명**: 현재 Unix 타임스탬프를 레지스터에 저장
**예시**:
```assembly
time %r0               # 현재 시간을 r0에 저장
```

#### `setz` - Zero Flag 설정
**문법**: `setz 목표_레지스터`
**설명**: zero_flag 값을 레지스터에 저장 (1 또는 0)
**예시**:
```assembly
cmp %r0, $10
setz %r1               # r1 = (r0 == 10 ? 1 : 0)
```

#### `debug` - 디버그 정보 출력
**문법**: `debug`
**설명**: 모든 레지스터의 현재 값을 출력
**예시**:
```assembly
debug
```

### 8. 시스템 호출 (System Calls)

#### `syscall` - 시스템 호출
**문법**: `syscall $호출_번호`
**설명**: 시스템 호출 수행

**지원하는 Syscall 번호**:

| 번호 | 이름 | 설명 |
|------|------|------|
| 0 | PUSH | r0을 스택에 푸시 |
| 1 | POP | 스택에서 팝하여 r0에 저장 |
| 2 | PRINT_INT | r0 값을 정수로 출력 |
| 3 | PRINT_CHAR | r0 값을 UTF-8 문자로 출력 |
| 4 | LOAD_MEM | r1 주소에서 값을 읽어 r0에 저장 |
| 5 | STORE_MEM | r1 주소에 r2 값을 저장 |
| 6 | READ_INPUT | 표준 입력에서 문자 읽어 r0에 저장 |
| 9 | NEWLINE | 개행 출력 |
| 10 | PRINT_HASH | r0을 16진수 형식으로 출력 |
| 11 | PRINT_DEC_HEX | r0을 10진수와 16진수로 출력 |
| 12 | PRINT_HEX | r0을 16진수로 출력 |
| 13 | PRINT_BIN | r0을 64비트 이진수로 출력 |
| 14 | INFO | RAM 크기 및 스택 포인터 정보 출력 |
| 15 | MEMCPY | r1(dest)에 r2(src)에서 r3(len) 바이트 복사 |
| 16 | MEMSET | r1(addr)부터 r3(len) 바이트를 r2(val)로 채우기 |
| 17 | DUMP_REGS | R0-R15 레지스터 값 출력 |

**예시**:
```assembly
mov $42, %r0
syscall $2             # 42 출력

mov $65, %r0
syscall $3             # 'A' 출력

syscall $9             # 개행 출력
```

### 9. 프로그램 제어 (Program Control)

#### `eop` - End Of Program
**문법**: `eop`
**설명**: 프로그램 종료 (반환값 0)
**예시**:
```assembly
eop
```

#### `eopv` - End Of Program with Value
**문법**: `eopv 레지스터` 또는 `eopv $값`
**설명**: 입력 값을 반환값으로 프로그램 종료
**예시**:
```assembly
mov $42, %r0
eopv %r0               # 42를 반환값으로 하여 프로그램 종료
eopv $100              # 100을 반환값으로 하여 프로그램 종료
```

---

## 프로그램 예제 (Examples)

### 예제 1: Hello World
```assembly
mov $72, %r0
syscall $3             # H 출력
mov $101, %r0
syscall $3             # e 출력
mov $108, %r0
syscall $3             # l 출력
syscall $3             # l 출력
mov $111, %r0
syscall $3             # o 출력
syscall $9             # 개행
eop
```

### 예제 2: 루프와 카운팅
```assembly
mov $0, %r0            # 카운터 = 0
mov $10, %r1           # 최대값 = 10
loop_start:
  mov %r0, %r2
  syscall $2           # 현재 값 출력
  syscall $9           # 개행
  inc %r0              # 카운터 증가
  cmp %r0, %r1
  jl loop_start        # 카운터 < 10이면 반복
eop
```

### 예제 3: 함수 호출
```assembly
call my_func
eop

my_func:
  mov $42, %r0
  syscall $2           # 42 출력
  syscall $9           # 개행
  ret
```

### 예제 4: 메모리 작업
```assembly
mov $100, %r0          # 값 = 100
mov $1000, %r1         # 주소 = 1000
store %r0, [%r1]       # 메모리[1000] = 100 저장

load [%r1], %r2        # r2 = 메모리[1000]
syscall $2             # 100 출력
eop
```

---

## 주석 (Comments)
- `#` 문자로 시작하는 줄은 주석
- 라인의 중간에서 시작한 주석도 지원

**예시**:
```assembly
mov $10, %r0     # r0에 10을 저장
# 다음은 덧셈 연산입니다
add $5, %r0      # r0 = r0 + 5 (결과: 15)
```

---

---

# English

## Overview
WVM (World Virtual Machine) supports AT&T syntax assembly language. This document is a comprehensive syntax guide for WVM assembly.

### Basic Syntax

#### Registers
- **Format**: `%r0` ~ `%r255`
- **Description**: 256 general-purpose 64-bit registers
- **Examples**: `%r0`, `%r1`, `%r15`

#### Immediate Values
- **Format**: Numbers or hexadecimal starting with `$`
- **Description**: Constant values
- **Examples**: `$10`, `$0x1000`, `$-100`

#### Memory Access
- **Format**: `[%r register_number]` or direct address
- **Description**: Read or write values from/to memory
- **Examples**: `[%r0]`, `[%r1]`

#### Labels
- **Format**: `label_name:`
- **Description**: Jump target location
- **Examples**: `loop:`, `start:`

### Architecture Information

- **Word Size**: 64-bit (8 bytes)
- **Architecture**: Hybrid stack and register based
- **Addressing Modes**: Immediate, register, direct memory address

---

## Instructions

### 1. Arithmetic Operations

#### `mov` - Move
**Syntax**: `mov source, destination`
**Description**: Copy value to destination
**Examples**:
```assembly
mov $100, %r0          # Copy 100 to r0
mov %r0, %r1          # Copy r0 to r1
mov $0x1000, %r2      # Copy hex value to r2
```

#### `add` - Addition
**Syntax**: `add source, destination` or `add source1, source2, destination`
**Description**: Add source values and store result in destination
**Examples**:
```assembly
add $10, %r0           # r0 = r0 + 10
add %r1, %r0          # r0 = r0 + r1
add %r1, $5, %r0      # r0 = r1 + 5
```

#### `sub` - Subtraction
**Syntax**: `sub source, destination` or `sub source1, source2, destination`
**Description**: Subtract sources and store result in destination
**Examples**:
```assembly
sub $5, %r0            # r0 = r0 - 5
sub %r1, %r0          # r0 = r0 - r1
```

#### `mul` - Multiplication
**Syntax**: `mul source, destination` or `mul source1, source2, destination`
**Description**: Multiply sources and store result in destination
**Examples**:
```assembly
mul $3, %r0            # r0 = r0 * 3
mul %r1, %r0          # r0 = r0 * r1
mul %r1, %r2, %r0     # r0 = r1 * r2
```

#### `div` - Division
**Syntax**: `div divisor, destination` or `div dividend, divisor, destination`
**Description**: Divide and store result in destination
**Examples**:
```assembly
div $2, %r0            # r0 = r0 / 2
div %r1, %r0          # r0 = r0 / r1
```

#### `inc` - Increment
**Syntax**: `inc register`
**Description**: Increment register value by 1
**Examples**:
```assembly
inc %r0                # r0 = r0 + 1
inc %r1
```

#### `dec` - Decrement
**Syntax**: `dec register`
**Description**: Decrement register value by 1
**Examples**:
```assembly
dec %r0                # r0 = r0 - 1
dec %r5
```

### 2. Bitwise Operations

#### `and` - Bitwise AND
**Syntax**: `and source, destination` or `and source1, source2, destination`
**Description**: Bitwise AND operation
**Examples**:
```assembly
and $0xFF, %r0         # r0 = r0 & 0xFF
and %r1, %r0          # r0 = r0 & r1
and %r1, $0xF0, %r0   # r0 = r1 & 0xF0
```

#### `or` - Bitwise OR
**Syntax**: `or source, destination` or `or source1, source2, destination`
**Description**: Bitwise OR operation
**Examples**:
```assembly
or $0x80, %r0          # r0 = r0 | 0x80
or %r1, %r0           # r0 = r0 | r1
```

#### `xor` - Bitwise XOR
**Syntax**: `xor source, destination` or `xor source1, source2, destination`
**Description**: Bitwise XOR operation
**Examples**:
```assembly
xor $0xFF, %r0         # r0 = r0 ^ 0xFF
xor %r1, %r0          # r0 = r0 ^ r1
```

#### `lsh` - Left Shift
**Syntax**: `lsh shift_amount, destination`
**Description**: Shift bits left
**Examples**:
```assembly
lsh $3, %r0            # r0 = r0 << 3
lsh %r1, %r0          # r0 = r0 << r1
```

#### `rsh` - Right Shift
**Syntax**: `rsh shift_amount, destination`
**Description**: Shift bits right
**Examples**:
```assembly
rsh $2, %r0            # r0 = r0 >> 2
rsh %r1, %r0          # r0 = r0 >> r1
```

### 3. Comparison & Branching

#### `cmp` - Compare
**Syntax**: `cmp source, target` or `cmp $immediate, register`
**Description**: Compare two values and set flags (zero_flag, sign_flag, carry_flag)
**Examples**:
```assembly
cmp %r0, $10           # Compare r0 with 10
cmp %r0, %r1          # Compare r0 with r1
```

#### `je` - Jump if Equal
**Syntax**: `je label` or `je register`
**Description**: Jump if zero_flag is set (a == b)
**Examples**:
```assembly
cmp %r0, $10
je equal_label
```

#### `jne` - Jump if Not Equal
**Syntax**: `jne label` or `jne register`
**Description**: Jump if zero_flag is not set (a != b)
**Examples**:
```assembly
cmp %r0, $5
jne not_equal_label
```

#### `jl` - Jump if Less
**Syntax**: `jl label` or `jl register`
**Description**: Jump if sign_flag is set (a < b)
**Examples**:
```assembly
cmp %r0, %r1
jl less_label
```

#### `jg` - Jump if Greater
**Syntax**: `jg label` or `jg register`
**Description**: Jump if sign_flag and zero_flag are not set (a > b)
**Examples**:
```assembly
cmp %r0, %r1
jg greater_label
```

#### `jle` - Jump if Less or Equal
**Syntax**: `jle label` or `jle register`
**Description**: Jump if sign_flag or zero_flag is set (a <= b)
**Examples**:
```assembly
cmp %r0, $100
jle less_equal_label
```

#### `jge` - Jump if Greater or Equal
**Syntax**: `jge label` or `jge register`
**Description**: Jump if sign_flag is not set (a >= b)
**Examples**:
```assembly
cmp %r0, $50
jge greater_equal_label
```

#### `jmp` - Unconditional Jump
**Syntax**: `jmp label` or `jmp register`
**Description**: Jump unconditionally to specified location
**Examples**:
```assembly
jmp start_label
jmp %r0                # Jump to address in r0
```

#### `loop` - Loop
**Syntax**: `loop label`
**Description**: Decrement register and jump if not zero
**Examples**:
```assembly
mov $10, %r2
loop_start:
  # Loop body
  loop loop_start      # Decrement r2, jump if not zero
```

### 4. Memory Operations

#### `load` - Load from Memory
**Syntax**: `load [address or register], destination_register`
**Description**: Read value from memory and store in register (8 bytes)
**Examples**:
```assembly
load [%r0], %r1        # Load from address in r0 to r1
load $1000, %r2       # Load from address 1000 to r2
```

#### `store` - Store to Memory
**Syntax**: `store source_register, [address or register]`
**Description**: Store register value to memory (8 bytes)
**Examples**:
```assembly
store %r0, [%r1]       # Store r0 to address in r1
store %r0, $2000      # Store r0 to address 2000
```

### 5. Stack Operations

#### `push` - Push to Stack
**Syntax**: `push source` or `push $immediate`
**Description**: Push value to stack (decrease stack pointer)
**Examples**:
```assembly
push %r0               # Push r0 to stack
push $100              # Push 100 to stack
```

#### `pop` - Pop from Stack
**Syntax**: `pop destination_register`
**Description**: Pop value from stack to register (increase stack pointer)
**Examples**:
```assembly
pop %r0                # Pop from stack to r0
pop %r1
```

### 6. Function Calls

#### `call` - Function Call
**Syntax**: `call label` or `call $address`
**Description**: Call function (push return address and jump)
**Examples**:
```assembly
call my_function
call $1000             # Call function at address 1000
```

#### `ret` - Function Return
**Syntax**: `ret`
**Description**: Return from function (pop return address and jump)
**Examples**:
```assembly
my_function:
  # Function body
  ret                  # Return to caller
```

### 7. Utility Instructions

#### `nop` - No Operation
**Syntax**: `nop`
**Description**: Do nothing
**Examples**:
```assembly
nop
```

#### `time` - Get Current Time
**Syntax**: `time destination_register`
**Description**: Store current Unix timestamp in register
**Examples**:
```assembly
time %r0               # Store current time in r0
```

#### `setz` - Set Zero Flag
**Syntax**: `setz destination_register`
**Description**: Store zero_flag value in register (1 or 0)
**Examples**:
```assembly
cmp %r0, $10
setz %r1               # r1 = (r0 == 10 ? 1 : 0)
```

#### `debug` - Print Debug Information
**Syntax**: `debug`
**Description**: Print all register values
**Examples**:
```assembly
debug
```

### 8. System Calls

#### `syscall` - System Call
**Syntax**: `syscall $call_number`
**Description**: Execute system call

**Supported Syscall Numbers**:

| Number | Name | Description |
|--------|------|-------------|
| 0 | PUSH | Push r0 to stack |
| 1 | POP | Pop from stack to r0 |
| 2 | PRINT_INT | Print r0 as integer |
| 3 | PRINT_CHAR | Print r0 as UTF-8 character |
| 4 | LOAD_MEM | Load from r1 address to r0 |
| 5 | STORE_MEM | Store r2 to address in r1 |
| 6 | READ_INPUT | Read character from stdin to r0 |
| 9 | NEWLINE | Print newline |
| 10 | PRINT_HASH | Print r0 as hex |
| 11 | PRINT_DEC_HEX | Print r0 as decimal and hex |
| 12 | PRINT_HEX | Print r0 as hex |
| 13 | PRINT_BIN | Print r0 as 64-bit binary |
| 14 | INFO | Print RAM size and stack pointer |
| 15 | MEMCPY | Copy r3 bytes from r2 (src) to r1 (dest) |
| 16 | MEMSET | Fill r3 bytes at r1 with value r2 |
| 17 | DUMP_REGS | Print R0-R15 register values |

**Examples**:
```assembly
mov $42, %r0
syscall $2             # Print 42

mov $65, %r0
syscall $3             # Print 'A'

syscall $9             # Print newline
```

### 9. Program Control

#### `eop` - End of Program
**Syntax**: `eop`
**Description**: Terminate program with return value 0
**Examples**:
```assembly
eop
```

#### `eopv` - End of Program with Value
**Syntax**: `eopv register` or `eopv $value`
**Description**: Terminate program with return value
**Examples**:
```assembly
mov $42, %r0
eopv %r0               # Exit with return value 42
eopv $100              # Exit with return value 100
```

---

## Program Examples

### Example 1: Hello World
```assembly
mov $72, %r0
syscall $3             # Print 'H'
mov $101, %r0
syscall $3             # Print 'e'
mov $108, %r0
syscall $3             # Print 'l'
syscall $3             # Print 'l'
mov $111, %r0
syscall $3             # Print 'o'
syscall $9             # Print newline
eop
```

### Example 2: Loop and Counting
```assembly
mov $0, %r0            # counter = 0
mov $10, %r1           # max = 10
loop_start:
  mov %r0, %r2
  syscall $2           # Print current value
  syscall $9           # Print newline
  inc %r0              # Increment counter
  cmp %r0, %r1
  jl loop_start        # Jump if counter < 10
eop
```

### Example 3: Function Call
```assembly
call my_func
eop

my_func:
  mov $42, %r0
  syscall $2           # Print 42
  syscall $9           # Print newline
  ret
```

### Example 4: Memory Operations
```assembly
mov $100, %r0          # value = 100
mov $1000, %r1         # address = 1000
store %r0, [%r1]       # memory[1000] = 100

load [%r1], %r2        # r2 = memory[1000]
syscall $2             # Print 100
eop
```

---

## Comments
- Lines starting with `#` are comments
- Comments can also start in the middle of a line

**Examples**:
```assembly
mov $10, %r0     # Store 10 in r0
# Next is addition
add $5, %r0      # r0 = r0 + 5 (result: 15)
```

