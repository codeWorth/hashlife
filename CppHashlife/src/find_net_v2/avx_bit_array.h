#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <algorithm>
#include <cstring>
#include <math.h>
#include <string>
#include <bitset>
#include <immintrin.h>
#include "bit_array.h"

#define CHUNK_DTYPE uint8_t // this MUST be an unsigned type
const uint32_t AVX_SIZE = 256;

#define AVX_LOAD(src) _mm256_load_si256(reinterpret_cast<const __m256i*>(src))
#define AVX_STORE(dest, src) _mm256_store_si256(reinterpret_cast<__m256i*>(dest), src)
#define ROT32(x, r) _mm256_or_si256(_mm256_slli_epi32(x, r), _mm256_srli_epi32(x, 32 - r))

const uint8_t RIGHT_SHIFT_0 = 0b11100100; // dst[3] = src[3], 2 = 2, 1 = 1, 0 = 0
const uint8_t RIGHT_SHIFT_1 = 0b10010011; // dst[3] = src[2], 2 = 1, 1 = 0, 0 = 0
const uint8_t RIGHT_SHIFT_2 = 0b01000000; // dst[3] = src[1], 2 = 0, 1 = 0, 0 = 0
const uint8_t RIGHT_SHIFT_3 = 0b00000000; // dst[3] = src[0], 2 = 0, 1 = 0, 0 = 0

const uint8_t LEFT_SHIFT_0 = 0b11100100; // 0: dst[3] = src[3], 2 = 2, 1 = 1, 0 = 0
const uint8_t LEFT_SHIFT_1 = 0b11111001; // 1: dst[3] = src[3], 2 = 3, 1 = 2, 0 = 1
const uint8_t LEFT_SHIFT_2 = 0b11111110; // 2: dst[3] = src[3], 2 = 3, 1 = 3, 0 = 2
const uint8_t LEFT_SHIFT_3 = 0b11111111; // 2: dst[3] = src[3], 2 = 3, 1 = 3, 0 = 3

const uint8_t BOTTOM_MASK_0 = 0b11111111;
const uint8_t BOTTOM_MASK_1 = 0b00111111;
const uint8_t BOTTOM_MASK_2 = 0b00001111;
const uint8_t BOTTOM_MASK_3 = 0b00000011;

const uint8_t TOP_MASK_0 = 0b11111111;
const uint8_t TOP_MASK_1 = 0b11111100;
const uint8_t TOP_MASK_2 = 0b11110000;
const uint8_t TOP_MASK_3 = 0b11000000;

// sublcass BitArray DOUBLES memory usage
// class AvxBitArray : public BitArray<AvxBitArray> {
class AvxBitArray {
private:
    __m256i chunks;

public:
    AvxBitArray(): AvxBitArray(true) {}
    AvxBitArray(bool high) {
        this->chunks = _mm256_set1_epi8(high ? 0xFF : 0x00);
    }

    AvxBitArray(const AvxBitArray &other) {
        this->chunks = other.chunks;
    }

    bool get(uint32_t index) const {
        uint32_t chunkIndex, subChunkIndex;
        chunkIndex = index / 8;
        subChunkIndex = index & 0b111;

        alignas(32) uint8_t values[AVX_SIZE / 8];
        AVX_STORE(values, chunks);

        return (values[chunkIndex] >> subChunkIndex) & 1;
    }

    void getAll(void *out) const {
        AVX_STORE(out, chunks);
    }

    void zero() {
        chunks = _mm256_set1_epi8(0x00);
    }

    void set(uint32_t index, bool bit) {
        uint32_t chunkIndex, subChunkIndex;
        chunkIndex = index / 8;
        subChunkIndex = index & 0b111;

        alignas(32) uint8_t values[AVX_SIZE / 8];
        AVX_STORE(values, chunks);

        if (bit) {
            values[chunkIndex] = (1 << subChunkIndex) | values[chunkIndex];
        } else {
            values[chunkIndex] = ~(1 << subChunkIndex) & values[chunkIndex];
        }
        
        chunks = AVX_LOAD(values);
    }

    void setAll(const AvxBitArray& other) {
        chunks = other.chunks;
    }

    bool none() const {
        auto cmp_result =_mm256_cmpeq_epi8(chunks, _mm256_set1_epi8(0x00));
        return _mm256_movemask_epi8(cmp_result) == 0xFFFFFFFF;
    }

    AvxBitArray* invert() {
        chunks = _mm256_xor_si256(chunks, _mm256_set1_epi8(0xFF));
        return this;
    }

    AvxBitArray operator~() const {
        AvxBitArray out(*this);
        out.invert();
        return out;
    }

    AvxBitArray& operator|=(const AvxBitArray& other) {
        chunks = _mm256_or_si256(chunks, other.chunks);
        return *this;
    }

    AvxBitArray operator|(const AvxBitArray& other) const {
        AvxBitArray out(*this);
        out |= other;
        return out;
    }

    AvxBitArray& operator&=(const AvxBitArray& other) {
        chunks = _mm256_and_si256(chunks, other.chunks);
        return *this;
    }

    AvxBitArray operator&(const AvxBitArray& other) const {
        AvxBitArray out(*this);
        out &= other;
        return out;
    }

    void and_out(const AvxBitArray& other, AvxBitArray& out) const {
        out.chunks = _mm256_and_si256(chunks, other.chunks);
    }

    AvxBitArray& operator^=(const AvxBitArray& other) {
        chunks = _mm256_xor_si256(chunks, other.chunks);
        return *this;
    }

    AvxBitArray operator^(const AvxBitArray& other) const {
        AvxBitArray out(*this);
        out ^= other;
        return out;
    }

    AvxBitArray& operator<<=(uint32_t amount) {
        if (amount >= AVX_SIZE) {
            zero();
            return *this;
        }

        uint32_t chunk_shift;
        chunk_shift = amount / 64; // number of full chunks we need to shift over
        amount -= chunk_shift * 64; // number of extra bits needed after the chunk shift happens

        auto rollover = _mm256_permute4x64_epi64(chunks, LEFT_SHIFT_1); // align the "next" chunks with the "current" (does not shfit in zeros)
        rollover = _mm256_slli_epi64(rollover, 64 - amount); // grab only the bits which will be missing after shift
        rollover = _mm256_blend_epi32(_mm256_set1_epi8(0x00), rollover, BOTTOM_MASK_1); // remove the garbage at the top

        chunks = _mm256_srli_epi64(chunks, amount); // do the main bitshift on the "current" chunks, it's a right shift because LSB = index 0, but LSB is on the right
        chunks = _mm256_or_si256(chunks, rollover); // copy in the bits which got missed at the top of each chunk

        switch (chunk_shift) {
        case 0:
            chunks = _mm256_permute4x64_epi64(chunks, LEFT_SHIFT_0); // shift left by the given number of chunks (does not shift in zeros)
            chunks = _mm256_blend_epi32(_mm256_set1_epi8(0x00), chunks, BOTTOM_MASK_0); // cut off the garabge
            break;

        case 1:
            chunks = _mm256_permute4x64_epi64(chunks, LEFT_SHIFT_1);
            chunks = _mm256_blend_epi32(_mm256_set1_epi8(0x00), chunks, BOTTOM_MASK_1);
            break;

        case 2:
            chunks = _mm256_permute4x64_epi64(chunks, LEFT_SHIFT_2);
            chunks = _mm256_blend_epi32(_mm256_set1_epi8(0x00), chunks, BOTTOM_MASK_2);
            break;

        case 3:
            chunks = _mm256_permute4x64_epi64(chunks, LEFT_SHIFT_3);
            chunks = _mm256_blend_epi32(_mm256_set1_epi8(0x00), chunks, BOTTOM_MASK_3);
            break;
        }

        return *this;
    }

    AvxBitArray operator<<(uint32_t amount) const {
        AvxBitArray out(*this);
        out <<= amount;
        return out;
    }

    AvxBitArray& operator>>=(uint32_t amount) {
        if (amount >= AVX_SIZE) {
            zero();
            return *this;
        }

        uint32_t chunk_shift;
        chunk_shift = amount / 64;
        amount -= chunk_shift * 64;

        auto rollover = _mm256_permute4x64_epi64(chunks, RIGHT_SHIFT_1);
        rollover = _mm256_srli_epi64(rollover, 64 - amount);
        rollover = _mm256_blend_epi32(_mm256_set1_epi8(0x00), rollover, TOP_MASK_1);

        chunks = _mm256_slli_epi64(chunks, amount);
        chunks = _mm256_or_si256(chunks, rollover);

        switch (chunk_shift) {
        case 0:
            chunks = _mm256_permute4x64_epi64(chunks, RIGHT_SHIFT_0);
            chunks = _mm256_blend_epi32(_mm256_set1_epi8(0x00), chunks, TOP_MASK_0);
            break;

        case 1:
            chunks = _mm256_permute4x64_epi64(chunks, RIGHT_SHIFT_1);
            chunks = _mm256_blend_epi32(_mm256_set1_epi8(0x00), chunks, TOP_MASK_1);
            break;

        case 2:
            chunks = _mm256_permute4x64_epi64(chunks, RIGHT_SHIFT_2);
            chunks = _mm256_blend_epi32(_mm256_set1_epi8(0x00), chunks, TOP_MASK_2);
            break;

        case 3:
            chunks = _mm256_permute4x64_epi64(chunks, RIGHT_SHIFT_3);
            chunks = _mm256_blend_epi32(_mm256_set1_epi8(0x00), chunks, TOP_MASK_3);
            break;
        }

        return *this;
    }

    AvxBitArray operator>>(uint32_t amount) const {
        AvxBitArray out(*this);
        out >>= amount;
        return out;
    }

    bool operator==(AvxBitArray const& other) const {
        auto cmp_result =_mm256_cmpeq_epi8(chunks, other.chunks);
        return _mm256_movemask_epi8(cmp_result) == 0xFFFFFFFF;
        return true;
    }

    bool operator<(AvxBitArray const& other) const {
        auto gt = _mm256_cmpgt_epi8(chunks, other.chunks);
        auto lt = _mm256_cmpgt_epi8(other.chunks, chunks);
        int gtX = _mm256_movemask_epi8(gt);
        int ltX = _mm256_movemask_epi8(lt);
        return gtX < ltX;
    }

    std::string toString() const {
        return toString(AVX_SIZE);
    }

    std::string toString(int size) const {
        const int printingChunkSize = 8;
        alignas(32) uint8_t values[AVX_SIZE / printingChunkSize];
        std::string out = "";
        std::string sep = " ";
        AVX_STORE(values, chunks);

        for (size_t i = 0; i < size / printingChunkSize; i++) {
            if (i != 0) out += sep;
            std::string bits = std::bitset<printingChunkSize>(values[i]).to_string();
            std::reverse(bits.begin(), bits.end());
            out += bits;
        }
        return out;
    }

    // adaptation of MurmurHash (https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp)
    uint64_t hash() const {
        const uint32_t seed = 0xdc5a1d43; // randomly generated
        __m256i hs = _mm256_set1_epi32(seed); // only index 0 and 4 will be used

        __m256i c1 = _mm256_set1_epi32(0xcc9e2d51);
        __m256i c2 = _mm256_set1_epi32(0x1b873593);
        __m256i c3 = _mm256_set1_epi32(0xe6546b64);
        __m256i five = _mm256_set1_epi32(5); // five

        auto blocks_reg = _mm256_mullo_epi32(chunks, c1);
        blocks_reg = ROT32(blocks_reg, 15);
        blocks_reg = _mm256_mullo_epi32(blocks_reg, c2);
        
        for(int i = 0; i < 4; i++) {
            hs = _mm256_xor_si256(hs, blocks_reg);
            hs = ROT32(hs, 13);
            hs = _mm256_mullo_epi32(hs, five);
            hs = _mm256_add_epi32(hs, c3);
            blocks_reg = _mm256_srli_si256(blocks_reg, 4);
        }

        hs = _mm256_xor_si256(hs, _mm256_srli_epi32(hs, 16));
        hs = _mm256_mullo_epi32(hs, _mm256_set1_epi32(0x85ebca6b));
        hs = _mm256_xor_si256(hs, _mm256_srli_epi32(hs, 13));
        hs = _mm256_mullo_epi32(hs, _mm256_set1_epi32(0xc2b2ae35));
        hs = _mm256_xor_si256(hs, _mm256_srli_epi32(hs, 16));

        uint64_t hLow = _mm256_extract_epi32(hs, 0);
        uint64_t hHigh = _mm256_extract_epi32(hs, 4);
        return (hHigh << 32) | (hLow & 0xFFFFFFFF);
    }
};