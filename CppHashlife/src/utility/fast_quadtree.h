#pragma once

#include <stdint.h>
#include <string.h>

#include "segmented_array.h"

template <class T>
class FastQuadtree {
public:
	FastQuadtree(uint8_t depth);

	struct iterator {
	public:
		iterator(FastQuadtree<T>* tree) : iterator(tree, 0) {};
		iterator(FastQuadtree<T>* tree, int index);
		~iterator();

		uint8_t height() const;
		int nodeIndex() const;
		int index() const;
		void nextValue();
		void nextNode();

		T& operator*();
		T* operator->();

	private:
		FastQuadtree<T>* tree;
		int _index;
		uint8_t* subindecies;
	};

	struct fast_iterator {
	public:
		fast_iterator(FastQuadtree<T>* tree) : tree(tree), _index(0) {};
		fast_iterator(FastQuadtree<T>* tree, int index) : tree(tree), _index(index) {};

		uint8_t height() const;
		int index() const;
		void nextValue();
		void nextNode();

		T& operator*();
		T* operator->();

	private:
		FastQuadtree<T>* tree;
		int _index;
	};

	void copyValues(int index, SegmentedArray<T>& values);

	iterator begin();
	iterator end();
	fast_iterator fast_begin();
	fast_iterator fast_end();

private:
	const uint8_t depth;
	const int totalLength;
	SegmentedArray<T> values;
	SegmentedArray<uint8_t> heights;

	int nodeCount(int depth) const;

};

template <class T>
FastQuadtree<T>::FastQuadtree(uint8_t depth) : 
	depth(depth), totalLength(nodeCount(depth)),
	values(totalLength), heights(totalLength)
{
	uint8_t* subindecies = new uint8_t[depth+1];
	for (int i = 0; i < depth+1; i++) {
		subindecies[i] = 0;
	}
	int height = depth;
	for (auto it = fast_begin(); it.index() < fast_end().index(); it.nextValue()) {
		heights[it.index()] = height;
		subindecies[height]++;
		if (height > 1) {
			height--;
		} else if (subindecies[height] == 4) {
			while (height < depth && subindecies[height] == 4) {
				subindecies[height] = 0;
				height++;
			}
		}
	}
	delete[] subindecies;
}

template <class T>
void FastQuadtree<T>::copyValues(int index, SegmentedArray<T>& values) {
	values.copyValues(index, values);
}

template <class T>
typename FastQuadtree<T>::iterator FastQuadtree<T>::begin() {
	return iterator(this);
}

template <class T>
typename FastQuadtree<T>::iterator FastQuadtree<T>::end() {
	return iterator(this, totalLength);
}

template <class T>
typename FastQuadtree<T>::fast_iterator FastQuadtree<T>::fast_begin() {
	return fast_iterator(this);
}

template <class T>
typename FastQuadtree<T>::fast_iterator FastQuadtree<T>::fast_end() {
	return fast_iterator(this, totalLength);
}

template <class T>
int FastQuadtree<T>::nodeCount(int depth) const {
	return ((1 << depth*2) - 1) / 3;
}





template <class T>
FastQuadtree<T>::iterator::iterator(FastQuadtree<T>* tree, int index) {
	this->tree = tree;
	this->_index = index;
	this->subindecies = new uint8_t[tree->depth+1];
	memset(this->subindecies, 0, tree->depth+1);
}

template <class T>
FastQuadtree<T>::iterator::~iterator() {
	delete[] subindecies;
}

template <class T>
uint8_t FastQuadtree<T>::iterator::height() const {
	return tree->heights[_index];
}

template <class T>
int FastQuadtree<T>::iterator::nodeIndex() const {
	return subindecies[height()];
}

template <class T>
int FastQuadtree<T>::iterator::index() const {
	return _index;
}

template <class T>
void FastQuadtree<T>::iterator::nextValue() {
	uint8_t oldHeight = height();
	_index++;
	uint8_t newHeight = height();

	subindecies[oldHeight]++;
	for (int i = oldHeight; i < newHeight; i++) {
		subindecies[i] = 0;
	}
}

template <class T>
void FastQuadtree<T>::iterator::nextNode() {
	uint8_t oldHeight = height();
	_index += tree->nodeCount(oldHeight);
	uint8_t newHeight = height();

	subindecies[oldHeight]++;
	for (int i = oldHeight; i < newHeight; i++) {
		subindecies[i] = 0;
	}
}

template <class T>
T& FastQuadtree<T>::iterator::operator*() {
	return tree->values[_index];
}

template <class T>
T* FastQuadtree<T>::iterator::operator->() {
	return &tree->values[_index];
}





template <class T>
uint8_t FastQuadtree<T>::fast_iterator::height() const {
	return tree->heights[_index];
}

template <class T>
int FastQuadtree<T>::fast_iterator::index() const {
	return _index;
}

template <class T>
void FastQuadtree<T>::fast_iterator::nextValue() {
	_index++;
}

template <class T>
void FastQuadtree<T>::fast_iterator::nextNode() {
	_index += tree->nodeCount(height());
}

template <class T>
T& FastQuadtree<T>::fast_iterator::operator*() {
	return tree->values[_index];
}

template <class T>
T* FastQuadtree<T>::fast_iterator::operator->() {
	return &tree->values[_index];
}