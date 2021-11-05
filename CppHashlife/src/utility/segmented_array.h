#pragma once

#include "math.h"
#include <stdint.h>
#include <algorithm>

template <class T>
class SegmentedArray {
public:
	const int size;

	SegmentedArray(int size);
	~SegmentedArray();

	T& operator[](int index);

	void copyValues(int destIndex, T* src, int count);
	void copyValues(int destIndex, SegmentedArray<T>& src);
	void copyValues(int destIndex, SegmentedArray<T>& src, int count);
	void copyValues(int destIndex, SegmentedArray<T>& src, int srcIndex, int count);

private:
	const uint8_t segmentDepth = 12;
	const int segmentLength = 1 << segmentDepth;
	const int segmentMask = (1 << segmentDepth) - 1;
	
	const int segmentCount;
	T** segments;

	void segmentIndex(int index, int& segmentIndex, int& subsegmentIndex) const;
	void copyValuesFromSegment(int destIndex, SegmentedArray<T>& src, int srcSegmentIndex, int count);

};

template <class T>
SegmentedArray<T>::SegmentedArray(int size) : 
	size(size),
	segmentCount(ceil((float)size / (float)segmentLength))
{
	segments = new T*[segmentCount];
	for (int i = 0; i < segmentCount-1; i++) {
		segments[i] = new T[segmentLength];
	}
	segments[segmentCount-1] = new T[size - (segmentCount-1)*segmentLength];
}

template <class T>
SegmentedArray<T>::~SegmentedArray() {
	for (int i = 0; i < segmentCount; i++) {
		delete[] segments[i];
	}
	delete[] segments;
}

template <class T>
T& SegmentedArray<T>::operator[](int index) {
	int i, j;
	segmentIndex(index, i, j);
	return segments[i][j];
}

template <class T>
void SegmentedArray<T>::copyValues(int destIndex, T* src, int count) {
	int segIndex, subsegIndex;
	segmentIndex(destIndex, segIndex, subsegIndex);

	// if the values we need to copy go over the edge of this segment
	if (subsegIndex + count > segmentLength) { 
		int segCount = segmentLength - subsegIndex; // copy to the end of this segment
		memcpy(&this->segments[segIndex][subsegIndex], src, segCount * sizeof(T));
		src += segCount;
		count -= segCount;
		segIndex++;

		// now the remaining values need to be copied to a segment alligned index
		while (count > 0) {
			segCount = std::min(segmentLength, count);
			memcpy(this->segments[segIndex], src, segCount * sizeof(T));
			count -= segCount;
			src += segCount;
			segIndex++;
		}
	} else {
		memcpy(&this->segments[segIndex][subsegIndex], src, count * sizeof(T));
	}
}

template <class T>
void SegmentedArray<T>::copyValues(int destIndex, SegmentedArray<T>& src) {
	copyValuesFromSegment(destIndex, src, 0, src.size);
}

template <class T>
void SegmentedArray<T>::copyValues(int destIndex, SegmentedArray<T>& src, int count) {
	copyValuesFromSegment(destIndex, src, 0, count);
}

template <class T>
void SegmentedArray<T>::copyValues(int destIndex, SegmentedArray<T>& src, int srcIndex, int count) {
	int segIndex, subsegIndex;
	segmentIndex(srcIndex, segIndex, subsegIndex);
	int copyLen = segmentLength - subsegIndex;

	// copy what we need from the first segment of src
	copyValues(destIndex, &src.segments[segIndex][subsegIndex], copyLen);
	destIndex += copyLen;
	count -= copyLen;

	// copy remaining segments of src
	copyValuesFromSegment(destIndex, src, segIndex+1, count);
}

template <class T>
void SegmentedArray<T>::segmentIndex(int index, int& segmentIndex, int& subsegmentIndex) const {
	segmentIndex = index >> segmentDepth;
	subsegmentIndex = index & segmentMask;
}

template <class T>
void SegmentedArray<T>::copyValuesFromSegment(int destIndex, SegmentedArray<T>& src, int srcSegmentIndex, int count) {
	int segIndex, subsegIndex, copyLen;
	segmentIndex(destIndex, segIndex, subsegIndex);

	int l1 = segmentLength - subsegIndex;
	int l2 = subsegIndex;

	while (count > 0) {
		// copy the first half of src's segment
		copyLen = std::min(count, l1);
		memcpy(&this->segments[segIndex][subsegIndex], src.segments[srcSegmentIndex], copyLen * sizeof(T));
		count -= copyLen;
		segIndex++;
		if (count <= 0) break;

		// copy the second half of src's segment
		copyLen = std::min(count, l2);
		memcpy(this->segments[segIndex], &src.segments[srcSegmentIndex][l1], copyLen * sizeof(T));
		count -= copyLen;
		srcSegmentIndex++;
	}
}