#pragma once

#include "avx_bit_array.h"
#include "compressed_bit_array.h"

class BitArrayCompressor {
public:
    CompressedBitArray compress(const AvxBitArray& bitArray) const {
        CompressedBitArray out(256, false);
        return out;
    }

    CompressedBitArray findable(const AvxBitArray& bitArray) const {
        CompressedBitArray out(bitArray.hash());
        return out;
    }
};