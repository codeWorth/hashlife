from __future__ import annotations
from dataclasses import dataclass
import math
from bit_array import BitArray

NEIGHBOR_COUNT = 8
TOTAL_PAIRS = (NEIGHBOR_COUNT - 1) * NEIGHBOR_COUNT // 2
PAIR_PERCENT = 1 / TOTAL_PAIRS
OUTPUT_SPACE_SIZE = 2 ** NEIGHBOR_COUNT
MAX_SWAPS = 19

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

def isOutputAllowed(output: int):
	neighbors = [0] * NEIGHBOR_COUNT
	for j in range(len(neighbors)):
		neighbors[j] = (output >> j) & 1

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

class OutputSpace:
	NEIGHBOR_LOW_MASKS = {}
	NEIGHBOR_HIGH_MASKS = {}
	for i in range(NEIGHBOR_COUNT):
		NEIGHBOR_LOW_MASKS[i] = BitArray(OUTPUT_SPACE_SIZE, False)
		NEIGHBOR_HIGH_MASKS[i] = BitArray(OUTPUT_SPACE_SIZE, False)
		for output in range(OUTPUT_SPACE_SIZE):
			NEIGHBOR_LOW_MASKS[i][output] = not((output >> i) & 1)
			NEIGHBOR_HIGH_MASKS[i][output] = (output >> i) & 1

	MASK_HASH_SHIFT = math.ceil(math.log2(NEIGHBOR_COUNT))
	PAIR_MASKS = {}
	I_PAIR_MASKS = {}

	ALLOWED_OUTPUT_SPACE = BitArray(OUTPUT_SPACE_SIZE, False)
	for output in range(OUTPUT_SPACE_SIZE):
		ALLOWED_OUTPUT_SPACE[output] = isOutputAllowed(output)

	# all bits must be equal to given neighbors (AND)
	@staticmethod
	def maskForNeighborBits(bits: dict[int, bool]) -> BitArray:
		# assert len(bits) > 0
		# assert len([i for i in bits if i >= NEIGHBOR_COUNT]) == 0
		mask = BitArray(OUTPUT_SPACE_SIZE, True)
		for index, value in bits.items():
			mask &= OutputSpace.NEIGHBOR_HIGH_MASKS[index] if value else OutputSpace.NEIGHBOR_LOW_MASKS[index]

		return mask

	@staticmethod
	def maskForPair(i: int, j: int) -> BitArray:
		h = (i << OutputSpace.MASK_HASH_SHIFT) + j
		if not(h in OutputSpace.PAIR_MASKS):
			OutputSpace.PAIR_MASKS[h] = OutputSpace.maskForNeighborBits({i: 0, j: 1})
		return OutputSpace.PAIR_MASKS[h]

	@staticmethod
	def iMaskForPair(i: int, j: int) -> BitArray:
		h = (i << OutputSpace.MASK_HASH_SHIFT) + j
		if not(h in OutputSpace.I_PAIR_MASKS):
			OutputSpace.I_PAIR_MASKS[h] = ~OutputSpace.maskForPair(i, j)
		return OutputSpace.I_PAIR_MASKS[h]

	def __init__(self, value: bool=True, space: BitArray=None):
		if space is None:
			self.space = BitArray(OUTPUT_SPACE_SIZE, value)
		else:
			self.space = space

	def cmpSwap(self, i: int, j: int):
		# assert i < j
		# assert j < NEIGHBOR_COUNT

		zeroOneMask = OutputSpace.maskForPair(i, j)
		zeroOneValues = (self.space & zeroOneMask) # only select ..0..1.. indices
		zeroOneValues <<= (2**j - 2**i) # this shift is equivalent of swaping i and j for each index 
		self.space |= zeroOneValues # selected ..1..0.. outputs are now possible
		self.space &= OutputSpace.iMaskForPair(i, j) # all ..0..1.. outputs are now impossible

	def willChange(self, i: int, j: int, mem: BitArray) -> bool:
		# assert i < j
		# assert j < NEIGHBOR_COUNT

		mem.setZeros()
		mem |= OutputSpace.maskForPair(i, j)
		mem &= self.space # only select ..0..1.. indices
		return not mem.fastAllZero()

	def isAllowed(self) -> bool:
		disallowed = ~OutputSpace.ALLOWED_OUTPUT_SPACE
		disallowed &= self.space
		return disallowed.fastAllZero()

	def __str__(self) -> str:
		return str(self.space)

	def hash(self) -> bytes:
		return self.space.hash()


@dataclass
class ExploredSpace:
	# when success is True, height is number of swaps to get from given state to success state
	# when success is False, height is the number of swaps tried from the given state 
	# for which none resulted in a success state
	height: int 
	swaps: list # list of swaps required to get from this state to success
	success: bool


@dataclass
class ExploredCount:
	count: int


# including the given pair
def remainingPairs(i: int, j: int) -> int:
	iLeft = NEIGHBOR_COUNT - i - 1
	return iLeft * (iLeft - 1) // 2 + NEIGHBOR_COUNT - j

TEMP_MEM = BitArray(OUTPUT_SPACE_SIZE)
def findSwaps(outputSpace: OutputSpace, swaps: list[tuple[int]], swaps_count: int, explored_output_spaces: dict[bytes, ExploredSpace], explored_count: ExploredCount):
	spaceHash = outputSpace.hash()
	if (spaceHash in explored_output_spaces):
		explored = explored_output_spaces[spaceHash]
		if (explored.success): return explored # successes in explored_output_spaces are always best possible
		if (not(explored.success) and explored.height >= MAX_SWAPS - swaps_count): return None # if we found a failure more thorough than we have time for, give up

	explored_count.count += 1
	if (explored_count.count & 0b1111111111111 == 0):
		percentDone = 0
		for k in range(swaps_count):
			percentDone += (TOTAL_PAIRS - remainingPairs(swaps[k][0], swaps[k][1])) * (PAIR_PERCENT ** (k + 1))

		percentDone = int(percentDone * 100 * 100) / 100
		print(f"Explored {explored_count.count} ({percentDone}%)")
		print(f"explored_out_spaces length = {len(explored_output_spaces)}")
		print(f"Current swaps: {swaps[:swaps_count]}")
		print()
		if (explored_count.count >= 80000):
			exit()

	if (outputSpace.isAllowed()):
		return ExploredSpace(0, [], True) # success

	if (swaps_count >= MAX_SWAPS):
		return None # failed

	foundPaths: list[ExploredSpace] = []
	for i in range(NEIGHBOR_COUNT - 1):
		for j in range(i+1, NEIGHBOR_COUNT):
			if (outputSpace.willChange(i, j, TEMP_MEM)):
				newOut = OutputSpace(space=outputSpace.space.copy())
				newOut.cmpSwap(i, j)
				swaps[swaps_count] = (i, j)
				maybeFound = findSwaps(newOut, swaps, swaps_count + 1, explored_output_spaces, explored_count)
				if (maybeFound is not None):
					found = ExploredSpace(maybeFound.height + 1, [(i, j)] + maybeFound.swaps, True)
					foundPaths.append(found)

	best_explored = None
	for explored in foundPaths:
		if best_explored is None or explored.height < best_explored.height:
			best_explored = explored

	if best_explored is not None:
		explored_output_spaces[spaceHash] = best_explored
		return best_explored
	elif spaceHash in explored_output_spaces:
		explored = explored_output_spaces[spaceHash]
		if (explored.height < MAX_SWAPS - swaps_count):
			explored_output_spaces[spaceHash].swaps = MAX_SWAPS - swaps_count
	else:
		explored_output_spaces[spaceHash] = ExploredSpace(MAX_SWAPS - swaps_count, None, False)

	return None

t1 = OutputSpace()
for i, j in CORRECT_SWAPS:
	t1.cmpSwap(i, j)

t2 = OutputSpace()
for i, j in CORRECT_SWAPS[:-5]:
	t2.cmpSwap(i, j)

if t1.isAllowed() and not(t2.isAllowed()):
	print("Passed sanity check")
else:
	print("Failed sanity check!")

swaps_count = 0
swaps = [None] * MAX_SWAPS
start_swaps = CORRECT_SWAPS[:8]
outputSpace = OutputSpace()
for i, j in start_swaps:
	outputSpace.cmpSwap(i, j)
	swaps[swaps_count] = (i, j)
	swaps_count += 1

results: dict[str, ExploredSpace] = {}
count = ExploredCount(0)
bestPath = findSwaps(outputSpace, swaps, swaps_count, results, count)
print("Explored (final)", count.count)
print("Best =", start_swaps + bestPath.swaps)