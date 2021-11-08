#pragma once
#include "grouped_bit_array.h"

class CompressedBitArray: public GroupedBitArray {
public:
    CompressedBitArray(uint64_t hash): GroupedBitArray() {
        this->chunks = new CHUNK_DTYPE[8 / sizeof(CHUNK_DTYPE)];
        ((uint64_t* )this->chunks)[0] = hash;
        this->size = 0;
    }

    CompressedBitArray(uint32_t size, bool value): GroupedBitArray(size, value) {}
};

class CompressedBitArrayHasher {
public:
    uint64_t operator()(const CompressedBitArray& key) const {
        return 0;
    }
};