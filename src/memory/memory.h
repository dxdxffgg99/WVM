#ifndef WVM_MEMORY_H
#define WVM_MEMORY_H

#include "ram/ram.h"
#include "register/register.h"

#define _8bit(v) program[i++] = (uint8_t)(v);

#define _64bit(v)                          \
(uint8_t)( ((uint64_t)(v)      ) & 0xFF ), \
(uint8_t)( ((uint64_t)(v) >> 8 ) & 0xFF ), \
(uint8_t)( ((uint64_t)(v) >> 16) & 0xFF ), \
(uint8_t)( ((uint64_t)(v) >> 24) & 0xFF ), \
(uint8_t)( ((uint64_t)(v) >> 32) & 0xFF ), \
(uint8_t)( ((uint64_t)(v) >> 40) & 0xFF ), \
(uint8_t)( ((uint64_t)(v) >> 48) & 0xFF ), \
(uint8_t)( ((uint64_t)(v) >> 56) & 0xFF )

#endif
