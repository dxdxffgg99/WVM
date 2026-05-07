mov $1000000000, %r0

loop:
  dec %r0
  cmp %r0, $0
  jne loop

eop