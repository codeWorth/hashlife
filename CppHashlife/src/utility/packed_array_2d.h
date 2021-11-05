#pragma once
#include <stdint.h>
#include "packed_array.h"

class PackedArray2D {
public:
	PackedArray2D(int wdith, int height);

	uint8_t get(int i, int j) const;
	uint8_t get(int k) const;
	void setLow(int i, int j);
	void setLow(int k);
	void setHigh(int i, int j);
	void setHigh(int k);
	void set(int i, int j, uint8_t bit);
	void set(int k, uint8_t bit);

	void print() const;

private:
	const int width;
	const int height;
	PackedArray array;

};