#pragma once
#include "compressed_bit_array.h"

class BitArrayCompressor {
public:
    CompressedBitArray compress(const AvxBitArray& bitArray) const {
        CompressedBitArray compressed(doCompress(bitArray).getData());
        return compressed;
    }

    CompressedBitArray findable(const AvxBitArray& bitArray) const {
        CompressedBitArray out(bitArray.hash(), &bitArray);
        return out;
    }

    void ensureInitialized(CompressedBitArray& bitArray) {
        if (bitArray.lazyGroupedBitArray.instantiated()) return;
        GroupedBitArray b = doCompress(bitArray.lazyGroupedBitArray.findable.uncompressedBitArray);
        bitArray.lazyGroupedBitArray.data = b.getData();
    }

private:
    GroupedBitArray doCompress(const AvxBitArray& bitArray) const {
        GroupedBitArray compressed(256);
        return compressed;
    }
};

class CompressedBitArrayEquality {
private:
    BitArrayCompressor* compressor;

public:
    CompressedBitArrayEquality(BitArrayCompressor* compressor): compressor(compressor) {}

    bool operator()(CompressedBitArray& key1, CompressedBitArray& key2) const {
        compressor->ensureInitialized(key1);
        compressor->ensureInitialized(key2);
        return GroupedBitArray(key1.lazyGroupedBitArray.data) == GroupedBitArray(key2.lazyGroupedBitArray.data);
    }
};