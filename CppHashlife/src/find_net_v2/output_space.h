#pragma once
#include <stdint.h>
#include <bitset>
#include "constants.h"
#include "mask_factory.h"

class OutputSpace {
private:
    std::bitset<OUTPUT_SPACE_SIZE>* allowedOutputSpace;
    MaskFactory* maskFactory;

public:
	OutputSpace(std::bitset<OUTPUT_SPACE_SIZE>* allowedOutputSpace, MaskFactory* maskFactory) {
        this->allowedOutputSpace = allowedOutputSpace;
        this->maskFactory = maskFactory;
    }

    void cmpSwap(uint32_t i, uint32_t j, std::bitset<OUTPUT_SPACE_SIZE>& mem) {
        
    }
};