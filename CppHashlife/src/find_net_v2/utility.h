#pragma once
#include <math.h>
#include "constants.h"

uint64_t nodesInTree(int k, int h) {
    return pow(k, h + 1) - 1 / (k - 1);
}

int remainingPairs(int i, int j) {
    int iLeft = NEIGHBOR_COUNT - i - 1;
    return iLeft * (iLeft - 1) / 2 + NEIGHBOR_COUNT - j;
}

void binaryToArray(int value, bool* out, int len) {
    for (size_t i = 0; i < len; i++) {
        out[i] = (value >> i) & 1;
    }
}