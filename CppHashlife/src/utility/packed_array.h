#pragma once
#include <stdint.h>

class PackedArray {
protected:
	uint32_t* values;
public:
	const int size;
	const uint16_t sections;
	static const int chunkPower = 5;
	static const int chunkSize = 1 << chunkPower;
	static const int lowerMask = 0b11111;

public:
	PackedArray(int size);
	~PackedArray();

	bool get(int index) const;
	void set(int index, uint8_t bit);
	void setLow(int index);
	void setHigh(int index);

	void print() const;
	
protected:
	PackedArray(int size, int padding);

};