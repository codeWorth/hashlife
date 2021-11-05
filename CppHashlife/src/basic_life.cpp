#include <random>

#include "basic_life.h"
#include "constants.h"

BasicLife::BasicLife(int size, std::random_device& rd) : size(size), eng(rd()), dist(0, 7) {
	this->cells = new uint8_t*[size+2];
	this->nextCells = new uint8_t*[size+2];
}

BasicLife::~BasicLife() {
	for (int i = 0; i < size+2; i++) {
		delete[] cells[i];
		delete[] nextCells[i];
	}
	delete[] cells;
	delete[] nextCells;
}

void BasicLife::setup() {
	for (int i = 0; i < size+2; i++) {
		cells[i] = new uint8_t[size+2];
		nextCells[i] = new uint8_t[size+2];

		for (int j = 0; j < size+2; j++) {
			cells[i][j] = 0x0;
			nextCells[i][j] = 0x0;
		}
	}

	for (int i = 1; i < size+1; i++) {
		for (int j = 1; j < size+1; j++) {
			if (dist(eng) == 0) {
				cells[i][j] = 0xFF;
			} else {
				cells[i][j] = 0x00;
			}
			nextCells[i][j] = cells[i][j];
		}
	}

}

void BasicLife::tick() {
	for (int i = 1; i < size+1; i++) {
		for (int j = 1; j < size+1; j++) {
			uint8_t neighborCount = 0;
			neighborCount += (cells[i-1][j-1]) ? 1 : 0;
			neighborCount += (cells[i  ][j-1]) ? 1 : 0;
			neighborCount += (cells[i+1][j-1]) ? 1 : 0;
			neighborCount += (cells[i-1][j  ]) ? 1 : 0;
			neighborCount += (cells[i+1][j  ]) ? 1 : 0;
			neighborCount += (cells[i-1][j+1]) ? 1 : 0;
			neighborCount += (cells[i  ][j+1]) ? 1 : 0;
			neighborCount += (cells[i+1][j+1]) ? 1 : 0;

			if (neighborCount == 3 || (cells[i][j] && neighborCount == 2)) {
				nextCells[i][j] = 0xFF;
			} else {
				nextCells[i][j] = 0x00;
			}
		}
	}

	swapMutex.lock();
	std::swap(cells, nextCells);
	swapMutex.unlock();
}

void BasicLife::draw(char* pixelBuffer) {
	swapMutex.lock();
	for (int i = 0; i < WINDOW_SIZE; i++) {
		memcpy(&pixelBuffer[i * WINDOW_SIZE], cells[i+1], WINDOW_SIZE);
	}
	swapMutex.unlock();
}