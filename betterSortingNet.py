from __future__ import annotations
from dataclasses import dataclass
import math
import numpy as np
import numba as nb

NEIGHBOR_COUNT = 8
TOTAL_PAIRS = (NEIGHBOR_COUNT - 1) * NEIGHBOR_COUNT // 2
PAIR_PERCENT = 1 / TOTAL_PAIRS
OUTPUT_SPACE_SIZE = 2 ** NEIGHBOR_COUNT
MAX_SWAPS = 19

# short-circuiting replacement for np.any()
@nb.jit(nopython=True)
def sc_any(array):
    for x in array.flat:
        if x:
            return True
    return False

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

class BitArray:
	def CHUNK_FORMAT(bits: int) -> str:
		return "{0:0" + str(bits) + "b}"

	def __init__(self, size: int, value: bool=False, chunks: np.ndarray=None, chunk_type=None):
		if chunk_type is None:
			chunk_type = np.uint8
		self.chunk_size = np.dtype(chunk_type).itemsize * 8

		self.size = size
		self.chunk_count = size // self.chunk_size
		if (size % self.chunk_size != 0):
			self.chunk_count += 1
		
		if chunks is None:
			self.chunks = np.empty(self.chunk_count, dtype=chunk_type)
			self.chunks[:] = 0xFFFFFFF if value else 0
		else:
			self.chunks = chunks

	def copy(self) -> BitArray:
		c = BitArray.__new__(BitArray)
		c.chunk_size = self.chunk_size
		c.size = self.size
		c.chunk_count = self.chunk_count
		c.chunks = self.chunks.copy() 
		return c

	def allZero(self) -> bool:
		extraBits = self.size - (self.chunk_count - 1) * self.chunk_size
		return (self.chunks[-1] & ((1 << extraBits) - 1)) == 0 and (self.chunks[:-1] == 0).all()

	# this will NOT handle size % self.chunk_size != 0 correctly
	def fastAllZero(self) -> bool:
		return not sc_any(self.chunks)

	# this will NOT handle size % self.chunk_size != 0 correctly
	def hash(self) -> bytes:
		return self.chunks.tobytes() 

	def canOp(self, other: BitArray) -> bool:
		return type(other) is BitArray and self.size == other.size and self.chunk_size == other.chunk_size

	def __or__(self, other: BitArray) -> BitArray:
		out = self.copy()
		out |= other
		return out

	def __and__(self, other: BitArray) -> BitArray:
		out = self.copy()
		out &= other
		return out

	def __xor__(self, other: BitArray) -> BitArray:
		out = self.copy()
		out ^= other
		return out

	def __invert__(self) -> BitArray:
		out = self.copy()
		out.invert()
		return out

	def __rshift__(self, amount: int) -> BitArray:
		out = self.copy()
		out >>= amount
		return out

	def __lshift__(self, amount: int) -> BitArray:
		out = self.copy()
		out <<= amount
		return out

	def __ior__(self, other: BitArray) -> BitArray:
		# assert self.canOp(other)
		np.bitwise_or(self.chunks, other.chunks, out=self.chunks)
		return self

	def __iand__(self, other: BitArray) -> BitArray:
		# assert self.canOp(other)
		np.bitwise_and(self.chunks, other.chunks, out=self.chunks)
		return self

	def __ixor__(self, other: BitArray) -> BitArray:
		# assert self.canOp(other)
		np.bitwise_xor(self.chunks, other.chunks, out=self.chunks)
		return self

	# this will NOT handle size % self.chunk_size != 0 correctly
	def __irshift__(self, amount: int) -> BitArray:
		if amount == 0: return self
		if amount >= self.size:
			self.chunks[:] = 0
			return self
		# assert amount > 0

		if amount < self.chunk_size:
			rolled = np.empty_like(self.chunks)
			rolled[1:] = self.chunks[:-1]
			rolled[0] = 0 
			np.right_shift(rolled, self.chunk_size - amount, out=rolled) # get rollover bits from adjacent chunks

			np.left_shift(self.chunks, amount, out=self.chunks) # main shift of each byte
			np.bitwise_or(self.chunks, rolled, out=self.chunks)
			return self
		else:
			rolls = amount // self.chunk_size
			self.chunks[rolls:] = self.chunks[:-rolls]
			self.chunks[:rolls] = 0
			return self.__irshift__(amount - rolls * self.chunk_size) # extra remaining shift

	# this will NOT handle size % self.chunk_size != 0 correctly
	def __ilshift__(self, amount: int) -> BitArray:
		if amount == 0: return self
		if amount >= self.size:
			self.chunks[:] = 0
			return self
		# assert amount > 0

		if amount < self.chunk_size:
			rolled = np.empty_like(self.chunks)
			rolled[:-1] = self.chunks[1:]
			rolled[-1] = 0 
			np.left_shift(rolled, self.chunk_size - amount, out=rolled)

			np.right_shift(self.chunks, amount, out=self.chunks)
			np.bitwise_or(self.chunks, rolled, out=self.chunks)
			return self
		else:
			rolls = amount // self.chunk_size
			self.chunks[:-rolls] = self.chunks[rolls:]
			self.chunks[-rolls:] = 0
			return self.__ilshift__(amount - rolls * self.chunk_size)

	def invert(self):
		np.invert(self.chunks, out=self.chunks)

	def __getitem__(self, key: int) -> bool:
		# assert key < self.size
		byteIndex, bitIndex = self.indices(key)
		return bool((self.chunks[byteIndex] >> bitIndex) & 1)

	def __setitem__(self, key: int, value: bool):
		# assert key < self.size
		byteIndex, bitIndex = self.indices(key)
		if value:
			self.chunks[byteIndex] = (1 << bitIndex) | self.chunks[byteIndex]
		else:
			self.chunks[byteIndex] = ~(1 << bitIndex) & self.chunks[byteIndex]

	def indices(self, key: int):
		byteIndex = key // self.chunk_size
		bitIndex = key - byteIndex * self.chunk_size
		return (byteIndex, bitIndex)

	def __str__(self) -> str:
		sep = ""
		out_str = sep.join(list(BitArray.CHUNK_FORMAT(self.chunk_size).format(v)[::-1] for v in self.chunks[:-1]))
		if (self.size > self.chunk_size):
			out_str += sep

		extra_bits = (self.size % self.chunk_size) or self.chunk_size
		return out_str + BitArray.CHUNK_FORMAT(extra_bits).format(self.chunks[-1])[::-1][:extra_bits]

	def __eq__(self, other: BitArray) -> bool:
		if type(other) is not BitArray: return False
		return np.array_equal(self.chunks, other.chunks)

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

	def willChange(self, i: int, j: int) -> bool:
		# assert i < j
		# assert j < NEIGHBOR_COUNT

		zeroOneMask = OutputSpace.maskForPair(i, j)
		zeroOneValues: BitArray = (self.space & zeroOneMask) # only select ..0..1.. indices
		return not zeroOneValues.fastAllZero()

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
			if (outputSpace.willChange(i, j)):
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
	elif spaceHash in explored_output_spaces:
		explored = explored_output_spaces[spaceHash]
		if (explored.height < MAX_SWAPS - swaps_count):
			explored_output_spaces[spaceHash].swaps = MAX_SWAPS - swaps_count
	else:
		explored_output_spaces[spaceHash] = ExploredSpace(MAX_SWAPS - swaps_count, None, False)

correct_swaps = [
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

testSpace = OutputSpace()
for i, j in correct_swaps:
	testSpace.cmpSwap(i, j)

if testSpace.isAllowed():
	print("Passed sanity check")
else:
	print("Failed sanity check!")

depth = 0
swaps = [None] * MAX_SWAPS
start_swaps = correct_swaps[:10]
outputSpace = OutputSpace()
for i, j in start_swaps:
	outputSpace.cmpSwap(i, j)
	swaps[depth] = (i, j)
	depth += 1

results: dict[str, ExploredSpace] = {}
count = ExploredCount(0)
findSwaps(outputSpace, swaps, depth, results, count)
print("Explored (final)", count.count)
print([result for result in results.values() if result.success and result.height > 7])

# s = 64
# t1 = BitArray(s, chunks=(np.random.rand(s // 8) * (2 ** 8)).astype(np.uint8), chunk_type=np.uint8)
# t2 = BitArray(s, chunk_type=np.uint16)
# for i in range(s):
# 	t2[i] = t1[i]

# for k in range(s):
# 	t1[i] = not(t1[i])
# 	t2[i] = not(t2[i])
# 	if str(t1) != str(t2):
# 		print("SAD")

# for k in range(s):
# 	if str(t1 << k) != str(t2 << k):
# 		print("SAD")
# 		break
