#include "packed_array.h"
#include "math.h"

#include <iostream>

PackedArray::PackedArray(int size) : 
	size(size),
	sections((uint16_t)ceil((float)size / (float)chunkSize)) 
{
	values = new uint32_t[sections];
}

PackedArray::PackedArray(int size, int padding) :
	size(size),
	sections((uint16_t)ceil((float)(size + padding) / (float)chunkSize)) 
{
	values = new uint32_t[sections];
}

PackedArray::~PackedArray() {
	delete[] values;
}

bool PackedArray::get(int index) const {
	int sectionIndex = index >> chunkPower;
	int subIndex = chunkSize - 1 - (index & lowerMask);
	return (values[sectionIndex] >> subIndex) & 1;
}

void PackedArray::setLow(int index) {
	int sectionIndex = index >> chunkPower;
	int subIndex = chunkSize - 1 - (index & lowerMask);
	values[sectionIndex] = values[sectionIndex] & (~(1 << subIndex));
}

void PackedArray::setHigh(int index) {
	int sectionIndex = index >> chunkPower;
	int subIndex = chunkSize - 1 - (index & lowerMask);
	values[sectionIndex] = values[sectionIndex] | (1 << subIndex);
}

void PackedArray::set(int index, uint8_t bit) {
	int sectionIndex = index >> chunkPower;
	int subIndex = chunkSize - 1 - (index & lowerMask);
	values[sectionIndex] = (values[sectionIndex] & (1 << subIndex)) | (bit << subIndex);
}

void PackedArray::print() const {
	for (int i = 0; i < size; i++) {
		if (get(i)) {
			std::cout << "1";
		} else {
			std::cout << "0";
		}
	}
	std::cout << std::endl;
}
