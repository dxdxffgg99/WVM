mov $1000000000, %r0

loop:
  dec %r0
  jne %r0, loop

eop