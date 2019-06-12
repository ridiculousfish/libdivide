#pragma once
#include "libdivide.h"

struct IntWithDivider {
    int value;
    libdivide::divider<int> divider;
};
