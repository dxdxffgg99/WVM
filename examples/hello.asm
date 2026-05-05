# Hello World Program

mov $72, %r0
syscall $3

mov $105, %r0
syscall $3

syscall $9

mov $15, %r0
mov $10, %r1
add %r1, %r0
syscall $2

syscall $9

eop

