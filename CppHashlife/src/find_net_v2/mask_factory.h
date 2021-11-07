#pragma once
#include "constants.h"

// based on testing, the bottom of the output space has the most variation, therefore it should be used as in the initial map
// outputspace[:4] -> outputspace[4:] -> explored_space object
// only do ^above if needed (run out of memory)

class MaskFactory {
private:


public:
    MaskFactory()
}