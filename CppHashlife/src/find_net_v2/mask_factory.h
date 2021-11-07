#pragma once
#include <unordered_map>
#include "constants.h"

// based on testing, the bottom of the output space has the most variation, therefore it should be used as in the initial map
// outputspace[:4] -> outputspace[4:] -> explored_space object
// only do ^above if needed (run out of memory)

struct ExploredSpace {
    uint8_t i;
    uint8_t j;
    uint8_t height;
    bool success;
};


class MaskFactory {
private:
    std::unordered_map<uint8_t[OUTPUT_SPACE_SIZE/8], ExploredSpace> lowMasks;

public:
    MaskFactory()
}