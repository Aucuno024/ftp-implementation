#include "utils.h"
#include <stdint.h>

int get_endianess() {
    static uint32_t one = 1;
    return ((* (uint8_t *) &one) == 0);
}
