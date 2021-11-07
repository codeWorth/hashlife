#pragma once
#include <unordered_map>
#include "constants.h"
#include "avx_bit_array.h"

// based on testing, the bottom of the output space has the most variation, therefore it should be used as in the initial map
// outputspace[:4] -> outputspace[4:] -> explored_space object
// only do ^above if needed (run out of memory)

class MaskFactory {
private:
    AvxBitArray highMasks[NEIGHBOR_COUNT];
    std::unordered_map<int, AvxBitArray> pairMasks;
    std::unordered_map<int, AvxBitArray> invertedPairMasks;

public:
    MaskFactory() {
        for (size_t i = 0; i < NEIGHBOR_COUNT; i++) {
            highMasks[i].zero();
            for (size_t output = 0; output < OUTPUT_SPACE_SIZE; output++) {
                highMasks[i].set(output, (output >> i) & 1);
            }
        }
        
        for (size_t i = 0; i < NEIGHBOR_COUNT - 1; i++) {
            for (size_t j = i + 1; j < NEIGHBOR_COUNT; j++) {
                std::unordered_map<int, bool> bitsMap;
                bitsMap[i] = false;
                bitsMap[j] = true;
                int mIndex = index(i, j);
                pairMasks[mIndex] = maskForNeighborBits(bitsMap);
                invertedPairMasks[mIndex] = ~pairMasks[mIndex];
            }
        }
    }

    const AvxBitArray& maskForPair(int i, int j) {
        return pairMasks[index(i, j)];
    }

    const AvxBitArray& invertedMaskForPair(int i, int j) {
        return invertedPairMasks[index(i, j)];
    }

private:
    int index(int i, int j) const {
        #ifdef DEBUG
            assert(i < j);
            assert(j < NEIGHBOR_COUNT);
        #endif
        return (i << NEIGHBOR_COUNT_LOG_2) + j;
    }

    AvxBitArray maskForNeighborBits(std::unordered_map<int, bool> bits) {
        AvxBitArray mask;
        for (auto& it: bits) {
            mask &= it.second ? highMasks[it.first] : ~highMasks[it.first];
        }
        return mask;
    }
};