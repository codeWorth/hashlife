#include <iostream>
#include <unordered_map>
#include <string.h>
#include <stdint.h>
#include <vector>
#include <bitset>
#include "math.h"

#include "../utility/packed_output.h"

using namespace std;

const int bits = 6;
const int maxDepth = 11;
const bool fullSort = false;

struct PairList {
	static const int maxPairs = 28;
	uint8_t pairCount;
	uint8_t depth;
	uint8_t is[maxPairs/2];
	uint8_t js[maxPairs/2];
	bool exact;

	PairList() : pairCount(0), depth(255), exact(false) {};

	void append(uint8_t i, uint8_t j) {
		if (pairCount < maxPairs) {
			uint8_t secInd = pairCount / 2;
			uint8_t subInd = pairCount & 1;
			if (subInd == 0) {
				is[secInd] = (is[secInd] & 0xF0) | (i & 0x0F);
				js[secInd] = (js[secInd] & 0xF0) | (j & 0x0F);
			} else {
				is[secInd] = (is[secInd] & 0x0F) | (i << 4);
				js[secInd] = (js[secInd] & 0x0F) | (j << 4);
			}
			pairCount++;
		} else {
			cout << "ran out of space in PairList (count = " << (int)pairCount << ")" << endl;
			pairCount++;
		}
	}
	uint8_t i(uint8_t index) const {
		uint8_t secInd = index / 2;
		uint8_t subInd = index & 1;
		if (subInd == 0) {
			return is[secInd] & 0x0F;
		} else {
			return (is[secInd] >> 4) & 0x0F; 
		}
	}
	uint8_t j(uint8_t index) const {
		uint8_t secInd = index / 2;
		uint8_t subInd = index & 1;
		if (subInd == 0) {
			return js[secInd] & 0x0F;
		} else {
			return (js[secInd] >> 4) & 0x0F; 
		}
	}
	void clear() {
		pairCount = 0;
	}
	void reset() {
		pairCount = 0;
		depth = 255;
		exact = false;
	}

	friend ostream& operator<<(ostream& os, const PairList& item) {
		os << "depth = " << (int)item.depth;
		for (int i = 0; i < item.pairCount; i++) {
			os << "\t" << (int)item.i(i) << "," << (int)item.j(i);
			
		}
		return os;
	}
};

struct StackItem {
	PackedOutput<bits> outputs;
	PairList minPairs;
	uint8_t i;
	uint8_t j;

	StackItem() : i(0), j(0) {};
	void reset() {
		i = 0;
		j = 0;
		minPairs.reset();
	}
};

struct OutputHash {
	size_t operator()(const PackedOutput<bits>& a) const {
		return a.hash();
	}
};

struct OutputCmp {
	bool operator()(const PackedOutput<bits>& a, const PackedOutput<bits>& b) const {
		return a.equals(b);
	}
};

bool sorted(bitset<bits>& b) {
	bool foundZero = false;
	for (int j = bits-1; j >= 0; j--) {
		if (b[j] == 0) {
			foundZero = true;
		} else if (b[j] == 1 && foundZero) {
			return false;
		}
	}
	return true;
}

bool halfSorted(bitset<bits>& b) {
	if (b[bits/2] == 0) {
		for (int i = 0; i < bits/2; i++) {
			if (b[i]) {
				return false;
			}
		}
	} else {
		bool foundZero = false;
		for (int j = bits-1; j >= bits/2; j--) {
			if (b[j] == 0) {
				foundZero = true;
			} else if (b[j] == 1 && foundZero) {
				return false;
			}
		}
	}
	return true;
}

PackedOutput<bits> getUnallowedOutputs() {
	vector<int> unallowed;
	PackedOutput<bits> outputs;

	for (int i = 0; i < outputs.size; i++) {
		bitset<bits> i_bits(i);
		if (fullSort) {
			if (!sorted(i_bits)) {
				unallowed.push_back(i);
			}
		} else {
			if (!halfSorted(i_bits)) {
				unallowed.push_back(i);
			}
		}
		
	}

	for (int i = 0; i < outputs.size; i++) {
		outputs.setLow(i);
	}
	for (int i = 0; i < unallowed.size(); i++) {
		outputs.setHigh(unallowed[i]);
	}

	return outputs;
}

int bestPath(
	unordered_map<PackedOutput<bits>, PairList, OutputHash, OutputCmp>& foundExact, 
	vector<pair<uint8_t, uint8_t>>& minPath,
	PackedOutput<bits>& prevState,
	int failures,
	uint8_t lastX, uint8_t lastY
) {
	PairList pairList = foundExact[prevState];
	int minFails = maxDepth*3;
	int minFailsIndex = -1;
	vector<pair<uint8_t, uint8_t>> nextPath;

	for (int i = 0; i < pairList.pairCount; i++) {
		uint8_t x = pairList.i(i);
		uint8_t y = pairList.j(i);
		PackedOutput<bits> nextState = prevState;
		nextState.swapMutateOutput(x, y);

		nextPath.clear();
		int mf = 0;
		if (pairList.depth > 1) {
			mf = bestPath(foundExact, nextPath, nextState, failures, x, y);
		}
		if (x == lastX || y == lastY || x == lastY || y == lastX) {
			mf++;
		}
		if (mf < minFails) {
			minFails = mf;
			minFailsIndex = i;
			minPath = nextPath;
		}
	}

	uint8_t x = pairList.i(minFailsIndex);
	uint8_t y = pairList.j(minFailsIndex);
	minPath.insert(minPath.begin(), {x, y});
	cout << "depth = " << (int)pairList.depth << "\t" << (int)x << "," << (int)y << endl;
	return minFails;
}

int main(int argc, char const *argv[]) {
	unordered_map<PackedOutput<bits>, uint8_t, OutputHash, OutputCmp> foundMin;
	unordered_map<PackedOutput<bits>, PairList, OutputHash, OutputCmp> foundExact;

	PackedOutput<bits> unallowed = getUnallowedOutputs();

	uint8_t combos[bits-1][bits];
	uint8_t k;
	for (int i = 0; i < bits-1; i++) {
		for (int j = i+1; j < bits; j++) {
			combos[i][j] = k;
			k++;
		}
	}

	unsigned long long iterCount = 0;
	int r = bits * (bits-1) / 2;
	unsigned long long iterTotal = (pow(r, maxDepth+1) - 1) / (r - 1) - 1;

	StackItem stack[maxDepth+2];
	uint8_t stackSize = 1;

	vector<pair<uint8_t, uint8_t>> swaps;
	// swaps[0] = {1, 3};
	// swaps[1] = {0, 2};
	// swaps[2] = {2, 3};
	// swaps[3] = {0, 1};
	// swaps[4] = {1, 2};
	// swaps[5] = {3, 6};
	// swaps[6] = {4, 5};

	for (int i = 0; i < swaps.size(); i++) {
		stack[0].outputs.swapMutateOutput(swaps[i].first, swaps[i].second);
	}

	while (true) {
		StackItem* item = stack + stackSize-1;
		uint8_t minDepth;
		uint8_t depth = stackSize-1;
		bool exact = false;

		if (item->outputs.nandAll(unallowed)) {
			stackSize--;
			minDepth = 0;
			exact = true;
		} else if (depth == maxDepth) {
			stackSize--;
			minDepth = 1;
		} else if (foundExact.count(item->outputs) > 0) {
			stackSize--;
			minDepth = foundExact[item->outputs].depth;
			exact = true;
		} else if (foundMin.count(item->outputs) > 0 && depth + foundMin[item->outputs] > maxDepth) {
			stackSize--;
			minDepth = foundMin[item->outputs];
		} else {
			item->j++;
			if (item->j >= bits) {
				item->i++;
				item->j = item->i + 1;
			}

			if (item->i >= bits-1) {
				minDepth = item->minPairs.depth;
				exact = item->minPairs.exact;
				if (exact) {
					foundExact.insert({item->outputs, item->minPairs});
				} else if (foundMin.count(item->outputs) == 0) {
					foundMin.insert({item->outputs, minDepth});
				} else {
					foundMin[item->outputs] = max(minDepth, foundMin[item->outputs]);
				}

				stackSize--;
				if (stackSize == 0) {
					break;
				}
			} else {
				iterCount++;
				if (iterCount % (1 << 20) == 0) {
					unsigned long long remain = iterTotal;
					for (int k = 0; k < stackSize; k++) {
						int x = stack[k].i;
						int y = stack[k].j;
						int pairCount = combos[x][y];
						remain -= pairCount * (pow(r, maxDepth-k) - 1) / (r - 1);

					}
					const int decimals = 100;
					remain *= decimals;
					float ratio = remain / iterTotal;
					float percentDone = (1.0f - ratio/decimals) * 100.0f;
					cout << percentDone << "% done" << endl;
				}
				
				StackItem* newItem = stack + stackSize;
				newItem->reset();
				newItem->outputs = item->outputs;
				newItem->outputs.swapMutateOutput(item->i, item->j);
				stackSize++;
				continue;
			}
		}

		if (stackSize == 0) {
			cout << "idk this prolly shouldn't happen" << endl;
			break;
		}
		item = stack + stackSize-1;
		minDepth++;
		if (exact && !item->minPairs.exact) {
			item->minPairs.clear();
			item->minPairs.depth = minDepth;
			item->minPairs.append(item->i, item->j);
			item->minPairs.exact = true;
		} else if (!exact && item->minPairs.exact) {
			continue;
		} else if (minDepth < item->minPairs.depth) {
			item->minPairs.clear();
			item->minPairs.depth = minDepth;
			item->minPairs.append(item->i, item->j);
		} else if (minDepth == item->minPairs.depth) {
			// item->minPairs.append(item->i, item->j);
		}
	}

	PackedOutput<bits> test;
	for (int i = 0; i < 10; i++) {
		cout << foundExact[test] << endl;
		test.swapMutateOutput(foundExact[test].i(0), foundExact[test].j(0));
	}
	if (test.nandAll(unallowed)) {
		cout << "test succeeded (in theory)..." << endl;
	} else {
		cout << "tes failed (in theory)..." << endl;
	}

	cout << iterCount << "/" << iterTotal << endl;
	PackedOutput<bits> outputs;
	for (int i = 0; i < swaps.size(); i++) {
		outputs.swapMutateOutput(swaps[i].first, swaps[i].second);
	}

	uint8_t lastX = 255;
	uint8_t lastY = 255;
	if (swaps.size() != 0) {
		lastX = swaps.back().first;
		lastY = swaps.back().second;
	}

	vector<pair<uint8_t, uint8_t>> bestSwaps;
	int failures = bestPath(foundExact, bestSwaps, outputs, 0, lastX, lastY);
	swaps.insert(swaps.end(), bestSwaps.begin(), bestSwaps.end());
	cout << failures << " failures" << endl;

	outputs = PackedOutput<bits>();
	for (int i = 0; i < swaps.size(); i++) {
		cout << (int)swaps[i].first << ", " << (int)swaps[i].second << endl;
		outputs.swapMutateOutput(swaps[i].first, swaps[i].second);
	}
	
	if (outputs.nandAll(unallowed)) {
		cout << "succeeded (in theory)..." << endl;
	} else {
		cout << "failed (in theory)..." << endl;
	}

	bool suc = true;
	for (int i = 0; i < 1 << bits; i++) {
		bitset<bits> i_bits(i);
		for (int j = 0; j < swaps.size(); j++) {
			int x = bits-1 - swaps[j].first;
			int y = bits-1 - swaps[j].second;
			if (i_bits[x] == 0 && i_bits[y] == 1) {
				i_bits[x] = 1;
				i_bits[y] = 0;
			}
		}

		if (fullSort) {
			if (!sorted(i_bits)) {
				cout << "FAIL: " << i_bits << endl;
				suc = false;
			}
		} else {
			if (!halfSorted(i_bits)) {
				cout << "FAIL: " << i_bits << endl;
				suc = false;
			}
		}
	}
	if (suc) {
		cout << "succeeded" << endl;
	}

	return 0;
}