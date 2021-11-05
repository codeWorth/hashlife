#include "packed_array_2d.h"
#include <iostream>

PackedArray2D::PackedArray2D(int wdith, int height) : width(width), height(height), array(width*height) {}

uint8_t PackedArray2D::get(int i, int j) const {
	return array.get(j*width + i);
}

uint8_t PackedArray2D::get(int k) const {
	return array.get(k);
}

void PackedArray2D::setLow(int i, int j) {
	array.setLow(j*width + i);
}

void PackedArray2D::setLow(int k) {
	array.setLow(k);
}

void PackedArray2D::setHigh(int i, int j) {
	array.setHigh(j*width + i);
}

void PackedArray2D::setHigh(int k) {
	array.setHigh(k);
}

void PackedArray2D::set(int i, int j, uint8_t bit) {
	array.set(j*width + i, bit);
}

void PackedArray2D::set(int k, uint8_t bit) {
	array.set(k, bit);
}

void PackedArray2D::print() const {
	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			std::cout << (int)get(i, j);
		}
		std::cout << std::endl;
	}
}