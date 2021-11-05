#pragma once
#include <iostream>

#include "packed_array.h"
#include "string.h"

template <uint8_t bits>
class PackedOutput : public PackedArray {
public:
	PackedOutput();
	PackedOutput(const PackedOutput<bits>& outputs);

	PackedOutput<bits>& operator=(const PackedOutput<bits>& outputs);

	void swapMutateOutput(uint8_t lowBit, uint8_t highBit);
	bool nandAll(const PackedOutput<bits>& other) const;
	uint32_t hash() const;
	bool equals(const PackedOutput<bits>& other) const;

	const char* asString() const;

};

template <uint8_t bits>
PackedOutput<bits>::PackedOutput() : PackedArray(1 << bits, 8) {
	memset(values, 0, sections * 4);
	for (int i = 0; i < size; i++) {
		setHigh(i);
	}
}

template <uint8_t bits>
PackedOutput<bits>::PackedOutput(const PackedOutput<bits>& outputs) : PackedArray(1 << bits, 8) {
	memcpy(values, outputs.values, sections * 4);
}

template <uint8_t bits>
PackedOutput<bits>& PackedOutput<bits>::operator=(const PackedOutput<bits>& outputs) {
	memcpy(values, outputs.values, sections * 4);
	return *this;
}

template <uint8_t bits>
void PackedOutput<bits>::swapMutateOutput(uint8_t lowBit, uint8_t highBit) {
	uint8_t zeroBit = bits-1-lowBit;
	uint8_t oneBit = bits-1-highBit;
	int index = 0;
	int invertBits = (1 << zeroBit) | (1 << oneBit);

	while (true) {
		index += index & (1 << zeroBit);
		index += (~index) & (1 << oneBit);
		if (index >= size) {
			break;
		}

		if (get(index)) {
			setHigh(index ^ invertBits);
			setLow(index);
		}
		index++;
	}
}

template <uint8_t bits>
bool PackedOutput<bits>::nandAll(const PackedOutput<bits>& other) const {
	uint32_t res = values[0] & other.values[0];
	for (int i = 1; i < size/32; i++) {
		res |= values[i] & other.values[i];
	}
	return !res;
}

template <uint8_t bits>
uint32_t PackedOutput<bits>::hash() const {
	uint32_t hash = values[0];
	for (int i = 1; i < size/32; i++) {
		hash += values[i];
		// hash = (hash << 5) - hash + values[i];
	}
	return hash;
}

template <uint8_t bits>
bool PackedOutput<bits>::equals(const PackedOutput<bits>& other) const {
	return strcmp(asString(), other.asString()) == 0;
}

template <uint8_t bits>
const char* PackedOutput<bits>::asString() const {
	return (char*)values;
}