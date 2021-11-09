#pragma once
#include <unordered_map>
#include <vector>
#include <iostream>
#include <assert.h>
#include "constants.h"
#include "avx_bit_array.h"
#include "mask_factory.h"
#include "utility.h"

#define LOGGING

#if defined(_WIN32) || defined(_WIN64) || (defined(__CYGWIN__) && !defined(_WIN32))
    #define WINDOWS
#endif

#ifdef WINDOWS
    #include "windows.h"
    #include "psapi.h"
#endif

struct Swap {
    uint8_t i;
    uint8_t j;

    Swap(): Swap(0, 0) {}
    Swap(int i, int j): i(i), j(j) {}
};

struct ExploredSpace {
    Swap swap;
    uint8_t height;
    bool success = false;
};

const int totalPairs = (NEIGHBOR_COUNT - 1) * NEIGHBOR_COUNT / 2;
double pairPercent = 1.0 / (double)totalPairs;

class NetFinder {
private:
    std::vector<Swap> startSwaps;
    int totalMaxSwaps;
    MaskFactory* maskFactory;
    AvxBitArray disallowedOutputSpace;
    AvxBitArray tempMemory;
    std::vector<AvxBitArray> stackTempMemory;

public:
    NetFinder(std::vector<Swap> startSwaps, int totalMaxSwaps, MaskFactory* maskFactory, AvxBitArray disallowedOutputSpace) {
        this->startSwaps = startSwaps;
        this->totalMaxSwaps = totalMaxSwaps;
        this->maskFactory = maskFactory;
        this->disallowedOutputSpace = disallowedOutputSpace;
    }

    std::vector<Swap> findBest() {
        for (int i = 0; i < totalMaxSwaps; i++) {
            AvxBitArray a;
            stackTempMemory.push_back(a);
        }

        uint64_t iterCount = 0;
        std::vector<Swap> swaps;
        std::unordered_map<AvxBitArray, ExploredSpace, AvxBitArrayHasher> exploredOutputSpaces;
        AvxBitArray outputSpace;
        for (int i = 0; i < startSwaps.size(); i++) {
            Swap swap(startSwaps[i].i, startSwaps[i].j);
            compareSwap(outputSpace, swap.i, swap.j);
            swaps.push_back(swap);
        }
        for (int i = startSwaps.size(); i < totalMaxSwaps; i++) {
            Swap a;
            swaps.push_back(a);
        }

        ExploredSpace best = findSwaps(outputSpace, swaps, startSwaps.size(), exploredOutputSpaces, iterCount, totalMaxSwaps);
        std::vector<Swap> goodSwaps;
        for (int i = 0; i < startSwaps.size(); i++) {
            goodSwaps.push_back(startSwaps[i]);
        }

        while (best.success && best.height > 0) {
            goodSwaps.push_back(best.swap);
            compareSwap(outputSpace, best.swap.i, best.swap.j);
            if (isAllowed(outputSpace)) {
                break;
            } else {
                best = exploredOutputSpaces[outputSpace];
            }
        }

        return goodSwaps;
    }

    void prettyPrint(const AvxBitArray& outputSpace) const {
        std::cout << "Allowed outputs:" << std::endl;
        bool n[8];
        for (int output = 0; output < OUTPUT_SPACE_SIZE; output++) {
            binaryToArray(output, n, 8);
            if (outputSpace.get(output)) {
                std::cout << "\t" << n[0] << n[1] << n[2] << n[3] << n[4] << n[5] << n[6] << n[7] << std::endl;
            }
        }
        std::cout << std::endl;
    }

public:
    ExploredSpace findSwaps(
        const AvxBitArray& outputSpace,
        std::vector<Swap>& swaps, // for logging only
        int swapsCount,
        std::unordered_map<AvxBitArray, ExploredSpace, AvxBitArrayHasher>& exploredOutputSpaces,
        uint64_t& iterCount,
        int maxSwaps
    ) {
        iterCount++;
        #ifdef LOGGING
            if (iterCount >= 0xFFFFFF) {
                iterCount = 0;
                uint64_t exploredCount = 0;
                double percentDone = 0;
                for (size_t k = startSwaps.size(); k < swapsCount; k++) {
                    uint64_t donePairs = totalPairs - remainingPairs(swaps[k].i, swaps[k].j);
                    percentDone += (double)donePairs * (pow(pairPercent, k - startSwaps.size() + 1));
                    exploredCount += donePairs * nodesInTree(NEIGHBOR_COUNT, totalMaxSwaps - k - 1);
                }
                
                percentDone = round(percentDone * 100.0 * 10000.0) / 10000.0;
                std::cout << "Explored " << exploredCount << " (" << percentDone << "%)" << std::endl;
                std::cout << "exploredOutputSpaces length = " << exploredOutputSpaces.size() << std::endl;
                #ifdef WINDOWS
                    PROCESS_MEMORY_COUNTERS_EX pmc;
                    GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
                    std::cout << "Currently used memory: " << (pmc.PrivateUsage / 1000000) << " mb" << std::endl;
                #endif
                std::cout << std::endl;
            }
        #endif

        ExploredSpace exploredOut;
        if (swapsCount > maxSwaps) {
            return exploredOut;
        }

        auto exploredIt = exploredOutputSpaces.find(outputSpace);
        if (exploredIt != exploredOutputSpaces.end()) {
            ExploredSpace explored = exploredIt->second;
            // fail if:
			// we find a success (which is best possible) which will take too many swaps OR
			// we find a failure which took as many (or more) swaps as we have time for, therefore we cannot find a success for this state
            bool failed = (explored.success && swapsCount + explored.height > maxSwaps) || (!explored.success && explored.height >= maxSwaps - swapsCount);
            if (failed) return exploredOut;
            if (explored.success) return explored; // successes in explored_output_spaces are always best possible, so exit immediately
        }

        if (isAllowed(outputSpace)) {
            exploredOut.success = true;
            exploredOut.height = 0;
            return exploredOut;
        }

        if (swapsCount == maxSwaps) return exploredOut;

        ExploredSpace found;
        for (uint8_t i = 0; i < NEIGHBOR_COUNT - 1; i++) {
            for (uint8_t j = i + 1; j < NEIGHBOR_COUNT; j++) {
                bool mightChange = (swapsCount == 0 || !(i == swaps[swapsCount-1].i && j == swaps[swapsCount-1].j));
                if (mightChange && willChange(outputSpace, i, j)) {
                    AvxBitArray newOutputSpace = stackTempMemory[swapsCount];
                    newOutputSpace.setAll(outputSpace);
                    compareSwap(newOutputSpace, i, j);
                    swaps[swapsCount].i = i;
                    swaps[swapsCount].j = j;
                    ExploredSpace maybeFound = findSwaps(newOutputSpace, swaps, swapsCount + 1, exploredOutputSpaces, iterCount, maxSwaps);
                    if (maybeFound.success) {
                        maxSwaps = swapsCount + maybeFound.height;
                        found.height = maybeFound.height + 1;
                        found.swap.i = i;
                        found.swap.j = j;
                        found.success = true;
                    }
                }
            }
        }

        if (found.success) {
            exploredOutputSpaces[outputSpace] = found;
            return found;
        }
        
        exploredIt = exploredOutputSpaces.find(outputSpace);
        if (exploredIt != exploredOutputSpaces.end()) {
            if (exploredIt->second.height < maxSwaps - swapsCount) {
                exploredIt->second.height = maxSwaps - swapsCount;
            }
        } else {
            exploredOutputSpaces[outputSpace].height = maxSwaps - swapsCount;
            exploredOutputSpaces[outputSpace].success = false;
        }

        return exploredOut;
    }

    void compareSwap(AvxBitArray& outputSpace, uint8_t i, uint8_t j) {
        #ifdef DEBUG
            assert(i < j);
            assert(j < NEIGHBOR_COUNT);
        #endif
        auto zeroOneMask = maskFactory->maskForPair(i, j);
        outputSpace.and_out(zeroOneMask, tempMemory);// only select ..0..1.. indices
        tempMemory <<= ((1 << j) - (1 << i)); // this shift is equivalent of swaping i and j for each index 
        outputSpace |= tempMemory; // selected ..1..0.. outputs are now possible
        outputSpace &= maskFactory->invertedMaskForPair(i, j); // all ..0..1.. outputs are now impossible
    }

    bool willChange(const AvxBitArray& outputSpace, uint8_t i, uint8_t j) {
        #ifdef DEBUG
            assert(i < j);
            assert(j < NEIGHBOR_COUNT);
        #endif
        outputSpace.and_out(maskFactory->maskForPair(i, j), tempMemory);
        return !tempMemory.none();
    }

    bool isAllowed(const AvxBitArray& outputSpace) {
        outputSpace.and_out(disallowedOutputSpace, tempMemory);
        return tempMemory.none();
    }
};