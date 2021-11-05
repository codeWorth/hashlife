#include "utility.h"

#include <bitset>
#include <iostream>

// swaps i and j if j > i
// this algo is invented by Astrid Yu, check out her website https://astrid.tech/
#define CMP_SWAP(i, j) { \
	auto x = neighbors[i]; \
	neighbors[i] = _mm256_or_si256(x, neighbors[j]); \
	neighbors[j] = _mm256_and_si256(x, neighbors[j]); \
} 

Utility::Utility() {
	this->leftBitMaskMem = (__m256i*)_mm_malloc(32, 32);
	this->rightBitMaskMem = (__m256i*)_mm_malloc(32, 32);
	this->removeLeftBitMem = (__m256i*)_mm_malloc(32, 32);
	this->removeRightBitMem = (__m256i*)_mm_malloc(32, 32);

	auto leftBitMask = _mm256_set1_epi16(0x0080);
	auto rightBitMask = _mm256_set1_epi16(0x0100);
	auto removeLeftBit = _mm256_set1_epi16(0xFF7F); // little endian means 0 must go in least significant byte
	auto removeRightBit = _mm256_set1_epi16(0xFEFF); // this time zero goes in most significant byte

	_mm256_store_si256(this->leftBitMaskMem, leftBitMask);
	_mm256_store_si256(this->rightBitMaskMem, rightBitMask);
	_mm256_store_si256(this->removeLeftBitMem, removeLeftBit);
	_mm256_store_si256(this->removeRightBitMem, removeRightBit);
}

Utility::~Utility() {
	_mm_free(leftBitMaskMem);
	_mm_free(rightBitMaskMem);
	_mm_free(removeLeftBitMem);
	_mm_free(removeRightBitMem);
}

void Utility::shiftLeft(uint8_t* rawBits, const __m256i& bits, __m256i& shiftedBits, bool rightmostBit) const {
	auto removeRightBit = _mm256_load_si256(removeRightBitMem);
	auto rightBitMask = _mm256_load_si256(rightBitMaskMem);

	auto leftedBits = _mm256_slli_epi16(bits, 1);					// rotate shorts left initially, will leave zero in wrong place because of little endian
	auto sadBits = _mm256_srli_epi16(bits, 15);						// get the bit which was put as a zero in line above
	auto leftNoBorder = _mm256_or_si256(leftedBits, sadBits);		// combine these two, filling the zero hole
	leftNoBorder = _mm256_and_si256(leftNoBorder, removeRightBit);	// mask out the last bit of each short (will be filled with next bit)

	auto borderBit = _mm256_and_si256(leftedBits, rightBitMask);	// grab the bit that needs to travel between short boundaries
	borderBit = _mm256_srli_si256(borderBit, 2); 					// for some reason rotate "right" actually rotates left, probably little-endianness again
	auto leftWithBorder = _mm256_or_si256(leftNoBorder, borderBit);	// fill most zero holes remaining

	// _mm256_srli_si256 rotates in 128 bit lanes, so the 128 boundary loses its bit, grab it manually
	uint64_t midBit = (rawBits[16] & 0x80) ? 0x0100000000000000LL : 0x0LL;
	uint64_t endBit = rightmostBit ? 0x0100000000000000LL : 0x0LL;
	auto holeVec = _mm256_set_epi64x(endBit, 0x0, midBit, 0x0);		// order of args is reversesd in _mm256_set_epi64x
	shiftedBits = _mm256_or_si256(leftWithBorder, holeVec);			// fill the final holes
}

void Utility::shiftRight(uint8_t* rawBits, const __m256i& bits, __m256i& shiftedBits, bool leftmostBit) const {
	auto removeLeftBit = _mm256_load_si256(removeLeftBitMem);
	auto leftBitMask = _mm256_load_si256(leftBitMaskMem);

	auto rightedBits = _mm256_srli_epi16(bits, 1);
	auto sadBits = _mm256_slli_epi16(bits, 15);
	auto rightNoBorder = _mm256_or_si256(rightedBits, sadBits);
	rightNoBorder = _mm256_and_si256(rightNoBorder, removeLeftBit);

	auto borderBit = _mm256_and_si256(rightedBits, leftBitMask);
	borderBit = _mm256_slli_si256(borderBit, 2); 				
	auto rightWithBorder = _mm256_or_si256(rightNoBorder, borderBit);

	uint64_t midBit = (rawBits[15] & 0x01) ? 0x0000000000000080LL : 0x0LL;
	uint64_t endBit = leftmostBit ? 0x0000000000000080LL : 0x0LL;
	auto holeVec = _mm256_set_epi64x(0x0, midBit, 0x0, endBit);
	shiftedBits = _mm256_or_si256(rightWithBorder, holeVec);
}

void Utility::nextState(uint8_t** cells, int row, int column, PackedArray2D& corners, __m256i& nextCells) const {
	auto state = _mm256_load_si256((__m256i *)&cells[row][column]);

	__m256i neighbors[8];
	neighbors[1] = _mm256_load_si256((__m256i *)(&cells[row-1][column])); 							// top middle
	neighbors[6] = _mm256_load_si256((__m256i *)(&cells[row+1][column])); 							// bottom middle
	shiftRight(&cells[row-1][column], neighbors[1], neighbors[0], (cells[row-1][column-1 ] & 0x01));// top left
	shiftLeft (&cells[row-1][column], neighbors[1], neighbors[2], (cells[row-1][column+32] & 0x80));// top right
	shiftRight(&cells[row  ][column], state,        neighbors[3], (cells[row  ][column-1 ] & 0x01));// middle left
	shiftLeft (&cells[row  ][column], state,        neighbors[4], (cells[row  ][column+32] & 0x80));// middle right
	shiftRight(&cells[row+1][column], neighbors[6], neighbors[5], (cells[row+1][column-1 ] & 0x01));// bottom left
	shiftLeft (&cells[row+1][column], neighbors[6], neighbors[7], (cells[row+1][column+32] & 0x80));// bottom right

	// for (int i = 0; i < 4; i++) {
	// 	for (int j = 7; j >= i+1; j--) {
	// 		CMP_SWAP(j-1, j);
	// 	}
	// }

	// CMP_SWAP(1, 3);
	// CMP_SWAP(0, 2);
	// CMP_SWAP(2, 3);
	// CMP_SWAP(0, 1);
	// CMP_SWAP(1, 2);
	// CMP_SWAP(3, 7);
	// CMP_SWAP(1, 4);
	// CMP_SWAP(3, 5);
	// CMP_SWAP(0, 3);
	// CMP_SWAP(2, 5);
	// CMP_SWAP(2, 6);
	// CMP_SWAP(1, 2);
	// CMP_SWAP(4, 6);
	// CMP_SWAP(2, 3);
	// CMP_SWAP(2, 4);
	// CMP_SWAP(3, 4);

	CMP_SWAP(3, 7);
	CMP_SWAP(2, 6);
	CMP_SWAP(1, 5);
	CMP_SWAP(0, 4);

	CMP_SWAP(5, 7);
	CMP_SWAP(4, 6);
	CMP_SWAP(1, 3);
	CMP_SWAP(0, 2);
	CMP_SWAP(3, 5);
	CMP_SWAP(2, 4);

	CMP_SWAP(6, 7);
	CMP_SWAP(4, 5);
	CMP_SWAP(2, 3);
	CMP_SWAP(0, 1);

	CMP_SWAP(3, 6);
	CMP_SWAP(1, 4);
	CMP_SWAP(5, 6);
	CMP_SWAP(3, 4);
	CMP_SWAP(1, 2);

	auto twoMaybeThree = _mm256_and_si256(neighbors[1], neighbors[0]);
	twoMaybeThree = _mm256_andnot_si256(neighbors[3], twoMaybeThree);
	auto exactlyThree = _mm256_and_si256(twoMaybeThree, neighbors[2]);

	// if we have exactly 3 neighbors we're def alive
	// if we have 2 neighbors, and we're alive, we stay alive
	nextCells =_mm256_or_si256(exactlyThree, _mm256_and_si256(twoMaybeThree, state)); 
}

void Utility::printm256i(const __m256i& val) const {
	alignas(16) uint8_t v[32];
	_mm256_storeu_si256((__m256i*)v, val);
	for (int i = 0; i < 32; i++) {
		std::bitset<8> b(v[i]);
		std::cout << b;
	}
	std::cout << std::endl;
}

void Utility::drawPackedRLE(const char* rle, int x, int y, bool invertX, bool invertY, uint8_t** cells) const {
	int charIndex = 0;
	int count = 0;
	int origX = x;

	uint8_t sbs[8] = {
		0b10000000,
		0b01000000,
		0b00100000,
		0b00010000,
		0b00001000,
		0b00000100,
		0b00000010,
		0b00000001,
	};

	while (true) {
		char c = rle[charIndex];
		if (c == '!') {
			break;
		}

		if (c == 'b') {
			if (count == 0) {
				count = 1;
			}
			x += count * (invertX ? -1 : 1);
			count = 0;
		} else if (c == 'o') {
			if (count == 0) {
				count = 1;
			}
			while (count > 0) {
				int byteIndex = x / 8;
				int subByteIndex = x - byteIndex * 8;
				cells[y+1][byteIndex+32] |= sbs[subByteIndex];

				x += (invertX ? -1 : 1);
				count--;
			}
		} else if (c == '$') {
			y += (invertY ? 1 : -1);
			x = origX;
		} else {
			int n = c - '0';
			count *= 10;
			count += n;
		}

		charIndex++;
	}
}