#pragma once

#include <immintrin.h>
#include <stdint.h>
#include "packed_array_2d.h"

class Utility {
public:
	Utility();
	~Utility();

	void nextState(uint8_t** cells, int row, int column, PackedArray2D& corners, __m256i& nextCells) const;
	void printm256i(const __m256i& val) const;
	void drawPackedRLE(const char* rle, int x, int y, bool invertX, bool invertY, uint8_t** cells) const;

private:
	__m256i* leftBitMaskMem;
	__m256i* rightBitMaskMem;
	__m256i* removeLeftBitMem;
	__m256i* removeRightBitMem;

	void shiftLeft(uint8_t* rawBits, const __m256i& bits, __m256i& shiftedBits, bool rightmostBit) const;
	void shiftRight(uint8_t* rawBits, const __m256i& bits, __m256i& shiftedBits, bool leftmostBit) const;

};