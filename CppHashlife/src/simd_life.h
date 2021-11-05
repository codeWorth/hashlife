#pragma once

#include <mutex>
#include <random>

#include "life.h"
#include "constants.h"
#include "utility/utility.h"
#include "utility/packed_array_2d.h"

class SIMDLife: Life {
public:
	SIMDLife(int size, std::random_device& rd);
	~SIMDLife();

	void setup();
	void tick();
	void draw(char* pixelBuffer);

private:
	
	const Utility util;
	std::default_random_engine eng;
	std::uniform_int_distribution<uint8_t> dist;
	const int size;
	const int rowLen;

	uint8_t** cells;
	uint8_t** nextCells;
	PackedArray2D corners;

	std::mutex swapMutex;
	uint8_t** drawCells;

};