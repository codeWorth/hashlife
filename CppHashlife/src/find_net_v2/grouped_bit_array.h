#pragma once
#include <stdint.h>
#include <assert.h>
#include <algorithm>
#include <cstring>
#include <math.h>
#include <string>
#include <bitset>
#include "bit_array.h"

#define DEBUG

#define CHUNK_DTYPE uint8_t // this MUST be an unsigned type
const uint32_t CHUNK_SIZE = sizeof(CHUNK_DTYPE) * 8;
const uint8_t CHUNK_SHIFT = (uint8_t) log2(CHUNK_SIZE); // rshift bit index by this to get index of chunk
const uint32_t CHUNK_MASK = CHUNK_SIZE - 1; // mask bit index by this to get sub-chunk index

class GroupedBitArray : public BitArray<GroupedBitArray> {
private:
	CHUNK_DTYPE* chunks;
	uint32_t size; // number of bits in the array
	uint32_t chunk_count;

public:
    GroupedBitArray(uint32_t size): GroupedBitArray(size, true) {}
	GroupedBitArray(uint32_t size, bool high) {
		#ifdef DEBUG
			assert(size % CHUNK_SIZE == 0);
			assert(size > 0);
		#endif
		this->size = size;
		this->chunk_count = size >> CHUNK_SHIFT;
		this->chunks = new CHUNK_DTYPE[this->chunk_count];
		std::fill(chunks, chunks + this->chunk_count, high ? -1 : 0);
	}

	GroupedBitArray(const GroupedBitArray &other) {
		this->size = other.size;
		this->chunk_count = other.chunk_count;
		this->chunks = new CHUNK_DTYPE[this->chunk_count];
		std::memcpy(this->chunks, other.chunks, this->size / 8);
	}

	~GroupedBitArray() {
		delete[] chunks;
	}

	bool get(uint32_t index) const {
		uint32_t chunkIndex, subChunkIndex;
		indices(index, chunkIndex, subChunkIndex);
		return (chunks[chunkIndex] >> subChunkIndex) & 1;
	}

	void zero() {
		std::memset(chunks, 0, size / 8);
	}

	void set(uint32_t index, bool bit) {
		uint32_t chunkIndex, subChunkIndex;
		indices(index, chunkIndex, subChunkIndex);
		if (bit) {
			chunks[chunkIndex] = (1 << subChunkIndex) | chunks[chunkIndex];
		} else {
			chunks[chunkIndex] = ~(1 << subChunkIndex) & chunks[chunkIndex];
		}
	}

	bool none() const {
		for (size_t i = 0; i < chunk_count; i++) {
			if (chunks[i] != 0) return false;
		}
		return true;
	}

	GroupedBitArray* invert() {
		for (size_t i = 0; i < chunk_count; i++) {
			chunks[i] = ~chunks[i];
		}
		return this;
	}

	GroupedBitArray operator~() const {
		GroupedBitArray out(*this);
		out.invert();
		return out;
	}

	GroupedBitArray& operator|=(const GroupedBitArray& other) {
		#ifdef DEBUG
			assert(canOp(other));
		#endif
		for (size_t i = 0; i < chunk_count; i++) {
			chunks[i] |= other.chunks[i];
		}
		return *this;
	}

	GroupedBitArray operator|(const GroupedBitArray& other) const {
		GroupedBitArray out(*this);
		out |= other;
		return out;
	}

	GroupedBitArray& operator&=(const GroupedBitArray& other) {
		#ifdef DEBUG
			assert(canOp(other));
		#endif
		for (size_t i = 0; i < chunk_count; i++) {
			chunks[i] &= other.chunks[i];
		}
		return *this;
	}

	GroupedBitArray operator&(const GroupedBitArray& other) const {
		GroupedBitArray out(*this);
		out &= other;
		return out;
	}

	void and_out(const GroupedBitArray& other, GroupedBitArray& out) const {
		#ifdef DEBUG
			assert(canOp(other));
			assert(canOp(out));
		#endif
		for (size_t i = 0; i < chunk_count; i++) {
			out.chunks[i] = chunks[i] & other.chunks[i];
		}
	}

	GroupedBitArray& operator^=(const GroupedBitArray& other) {
		#ifdef DEBUG
			assert(canOp(other));
		#endif
		for (size_t i = 0; i < chunk_count; i++) {
			chunks[i] ^= other.chunks[i];
		}
		return *this;
	}

	GroupedBitArray operator^(const GroupedBitArray& other) const {
		GroupedBitArray out(*this);
		out ^= other;
		return out;
	}

	GroupedBitArray& operator<<=(uint32_t amount) {
		#ifdef DEBUG
			assert(amount >= 0);
		#endif
		if (amount >= size) {
			zero();
			return *this;
		}

		if (amount >= CHUNK_SIZE) {
			uint32_t rolls;
			indices(amount, rolls, amount);
			rollLeft(rolls);
		}

		if (amount == 0) return *this;

		for (size_t i = 0; i < chunk_count - 1; i++) {
			CHUNK_DTYPE rollover = chunks[i + 1] << (CHUNK_SIZE - amount);
			chunks[i] >>= amount;
			chunks[i] |= rollover;
		}
		chunks[chunk_count - 1] >>= amount;

		return *this;
	}

	GroupedBitArray operator<<(uint32_t amount) const {
		GroupedBitArray out(*this);
		out <<= amount;
		return out;
	}

	GroupedBitArray& operator>>=(uint32_t amount) {
		#ifdef DEBUG
			assert(amount >= 0);
		#endif
		if (amount >= size) {
			zero();
			return *this;
		}

		if (amount >= CHUNK_SIZE) {
			uint32_t rolls;
			indices(amount, rolls, amount);
			rollRight(rolls);
		}

		if (amount == 0) return *this;

		for (size_t i = 1; i < chunk_count; i++) {
			CHUNK_DTYPE rollover = chunks[i - 1] >> (CHUNK_SIZE - amount);
			chunks[i] <<= amount;
			chunks[i] |= rollover;
		}
		chunks[0] <<= amount;

		return *this;
	}

	GroupedBitArray operator>>(uint32_t amount) const {
		GroupedBitArray out(*this);
		out >>= amount;
		return out;
	}

	bool operator==(GroupedBitArray const& other) const {
		#ifdef DEBUG
			assert(canOp(other));
		#endif
		for (size_t i = 0; i < chunk_count; i++) {
			if (chunks[i] != other.chunks[i]) return false;
		}
		return true;
	}

	std::string toString() const {
		return toString(chunk_count);
	}

	std::string toString(int size) const {
		std::string out = "";
        std::string sep = " ";
		for (size_t i = 0; i < size; i++) {
            if (i != 0) out += sep;
            std::string bits = std::bitset<CHUNK_SIZE>(chunks[i]).to_string();
            std::reverse(bits.begin(), bits.end());
			out += bits;
		}
		return out;
	}
	
private:
	void indices(uint32_t index, uint32_t& chunkIndex, uint32_t& subChunkIndex) const {
		chunkIndex = index >> CHUNK_SHIFT;
		subChunkIndex = index & CHUNK_MASK;
	}

	bool canOp(const GroupedBitArray& other) const {
		return size == other.size;
	}

	void rollLeft(uint32_t amount) {
		for (size_t i = amount; i < chunk_count; i++) {
			chunks[i - amount] = chunks[i];
		}
		std::memset(chunks + chunk_count - amount, 0, amount << (CHUNK_SHIFT - 3));
	}

	void rollRight(uint32_t amount) {
		for (size_t i = amount; i < chunk_count; i++) {
			chunks[i] = chunks[i - amount];
		}
		std::memset(chunks, 0, amount << (CHUNK_SHIFT - 3));
	}
};