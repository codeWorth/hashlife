#pragma once
#include "grouped_bit_array.h"
#include "avx_bit_array.h"

class CompressedBitArray {
public:
    GroupedBitArray::Lazy lazyGroupedBitArray;

public:
    CompressedBitArray(uint64_t hash, const AvxBitArray* bitArray) {
        this->lazyGroupedBitArray.findable.hash = hash;
        this->lazyGroupedBitArray.findable.uncompressedBitArray = bitArray;
    }

    CompressedBitArray(GroupedBitArray::Data data) {
        this->lazyGroupedBitArray.data = data;
    }
};

class CompressedBitArrayHasher {
public:
    uint64_t operator()(const CompressedBitArray& key) const {
        return 0;
    }
};