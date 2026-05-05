mov $1000000000, %r0

loop_start:
  dec %r0
  cmp %r0, $0
  jne loop_start

eop