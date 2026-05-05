# Loop Example

mov $0, %r0
mov $10, %r1

loop_start:
  mov %r0, %r2
  syscall $2
  syscall $9
  inc %r0
  cmp %r0, %r1
  jl loop_start

eop

