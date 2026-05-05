mov $1, %r0
mov $1, %r1
mov $15, %r2
sub $1, %r2

loop:
    mov %r1, %r3
    add %r0, %r1
    mov %r3, %r0
    sub $1, %r2
    cmp $0, %r2
    jg loop

syscall $2
syscall $9
eop