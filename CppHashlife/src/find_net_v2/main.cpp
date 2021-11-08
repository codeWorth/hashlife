#include "net_finder.h"
#include <iostream>

#include "grouped_bit_array.h"

using namespace std;

#define IS_ALLOWED conwayAllowed

bool conwayAllowed(bool* neighbors) {
    int neighborCount = 0;
    for (int i = 0; i < 8; i++) {
        if (neighbors[i]) neighborCount++;
    }

    for (int state = 0; state < 2; state++) {
        // get state according to robust conways rules
        bool wantedState = false;
        if (state == 1 && neighborCount >= 2 && neighborCount <= 3) {
            wantedState = true;
        } else if (neighborCount == 3) {
            wantedState = true;
        }

        // get bit magic result for this set of neighbors
        bool gte2_lte3 = neighbors[0] && neighbors[1] && !neighbors[3];
		bool eq3 = neighbors[2] && !neighbors[3];
		bool newState = eq3 || (gte2_lte3 && state);

        // fail out if bit magic result doesn't match correct result
        if (newState != wantedState) return false;
    }

    return true;
}

int main(int argc, char *argv[]) {
    std::cout << "GroupedBitArray: " << sizeof(GroupedBitArray) << std::endl;
    std::cout << "AvxBitArray: " << sizeof(AvxBitArray) << std::endl;
    std::cout << "__m256i: " << sizeof(__m256i) << std::endl;

    return 0;

    AvxBitArray allowedOutputSpace;
    bool neighbors[NEIGHBOR_COUNT];
    for (int output = 0; output < OUTPUT_SPACE_SIZE; output++) {
        binaryToArray(output, neighbors, NEIGHBOR_COUNT);
        allowedOutputSpace.set(output, IS_ALLOWED(neighbors));
    }

    std::vector<Swap> correctSwaps;
    correctSwaps.push_back(Swap(0, 4));
    correctSwaps.push_back(Swap(1, 5));
    correctSwaps.push_back(Swap(2, 6));
    // correctSwaps.push_back(Swap(3, 7));
    // correctSwaps.push_back(Swap(0, 2));
    // correctSwaps.push_back(Swap(1, 3));
    // correctSwaps.push_back(Swap(4, 6));
    // correctSwaps.push_back(Swap(5, 7));
    // correctSwaps.push_back(Swap(2, 4));
    // correctSwaps.push_back(Swap(3, 5));
    // correctSwaps.push_back(Swap(0, 1));
    // correctSwaps.push_back(Swap(2, 3));
    // correctSwaps.push_back(Swap(4, 5));
    // correctSwaps.push_back(Swap(6, 7));
    // correctSwaps.push_back(Swap(1, 4));
    // correctSwaps.push_back(Swap(3, 6));
    // correctSwaps.push_back(Swap(1, 2));
    // correctSwaps.push_back(Swap(3, 4));
    // correctSwaps.push_back(Swap(5, 6));

    MaskFactory maskFactory;
    NetFinder finder(correctSwaps, 17, &maskFactory, ~allowedOutputSpace);
    std::vector<Swap> gotSwaps = finder.findBest();

    cout << "Height: " << gotSwaps.size() << endl;
    for (int i = 0; i < gotSwaps.size(); i++) {
        cout << (int)gotSwaps[i].i << ", " << (int)gotSwaps[i].j << endl;
    }
}