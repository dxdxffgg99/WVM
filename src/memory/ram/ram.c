#include "ram.h"
#include <stdlib.h>
#include <stdbool.h>

bool ram_init(RAM *ram, uint64_t size) {
    ram->data = (uint8_t *) malloc(size);
    if (!ram->data) {
        return false;
    }
    ram->size = size;
    return true;
}

void ram_free(RAM *ram) {
    if (ram->data) {
        free(ram->data);
        ram->data = NULL;
    }
    ram->size = 0;
}