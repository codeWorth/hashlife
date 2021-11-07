from typing import Callable
import numpy as np
from bit_array import BitArray
from util import NEIGHBOR_COUNT, binaryToArray
from betterSortingNet import NetFinder
from test_swap import does_conway

OUTPUT_SPACE_SIZE = 2 ** NEIGHBOR_COUNT

def getForIsAllowed(isAllowed: Callable[[list[bool]], bool]) -> BitArray:
	allowedOutputSpace = BitArray(OUTPUT_SPACE_SIZE)
	neighbors = [0] * NEIGHBOR_COUNT
	for output in range(OUTPUT_SPACE_SIZE):
		binaryToArray(output, neighbors)
		allowedOutputSpace[output] = isAllowed(neighbors)

	return allowedOutputSpace

def evacuateTop4(neighbors: list[bool]) -> bool:
	bottom4Count = len(list(k for k in neighbors[:4] if k == 1))
	top4Count = len(list(k for k in neighbors[4:] if k == 1))

	return bottom4Count == 4 or top4Count == 0

def conway(neighbors: list[bool]) -> bool:
	neighborCount = len(list(k for k in neighbors if k == 1))
	for state in range(2):
		# get state according to robust conways rules
		wantedState = 0
		if (state == 1 and neighborCount >= 2 and neighborCount <= 3):
			wantedState = 1
		elif (neighborCount == 3):
			wantedState = 1

		# get bit magic result for this set of neighbors
		gte2_lte3 = (neighbors[0] & neighbors[1] & ~neighbors[3]) & 1
		eq3 = (neighbors[2] & ~neighbors[3]) & 1
		newState = (eq3 | (gte2_lte3 & state)) & 1

		# fail out if bit magic result doesn't match correct result
		if (newState != wantedState): return False

	return True

def testTop4():
	finder = NetFinder([(0, 2), (1, 3), (0, 1), (2, 3)], 14, NEIGHBOR_COUNT, getForIsAllowed(evacuateTop4))
	print(finder.tryFind())

def testConway():
	CORRECT_SWAPS = [
		(0, 4),
		(1, 5),
		(2, 6),
		(3, 7),
		(0, 2),
		(1, 3),
		(4, 6),
		(5, 7),
		(2, 4),
		(3, 5),
		(0, 1),
		(2, 3),
		(4, 5),
		(6, 7),
		(1, 4),
		(3, 6),
		(1, 2),
		(3, 4),
		(5, 6),
	]
	MAX_SWAPS = len(CORRECT_SWAPS)
	INITIAL_SWAPS = 9

	finder = NetFinder(CORRECT_SWAPS[:INITIAL_SWAPS], MAX_SWAPS, NEIGHBOR_COUNT, getForIsAllowed(conway))
	bestPath = finder.tryFind()
	print(f"Best ({len(bestPath)}) = {bestPath}")
	print("Good?", does_conway(bestPath))

if __name__ == "__main__":
	testConway()