#include <math.h>

#include "utils.h"

int decimal_len(size_t size)
{
    if (size == 0)
        return 1;

    return floor(log10(size)) + 1;
}
