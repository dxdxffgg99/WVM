# Arithmetic Operations Example

mov $10, %r0
mov $5, %r1

add %r1, %r0
syscall $2
syscall $9

mov $20, %r0
mov $3, %r1

mul %r1, %r0
syscall $2
syscall $9

mov $100, %r0
mov $4, %r1

div %r1, %r0
syscall $2
syscall $9

eop

