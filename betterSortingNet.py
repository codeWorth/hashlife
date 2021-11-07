from __future__ import annotations
from dataclasses import dataclass
import math
from bit_array import BitArray
from util import NEIGHBOR_COUNT, nodesInTree, remainingPairs, binaryToArray

class NeighborMaskFactory:
	def __init__(self, neighbor_count: int, output_space_size: int):
		self.neighbor_count = neighbor_count
		self.output_space_size = output_space_size

		self.neighbor_low_masks = {}
		self.neighbor_high_masks = {}
		for i in range(neighbor_count):
			self.neighbor_low_masks[i] = BitArray(output_space_size, False)
			self.neighbor_high_masks[i] = BitArray(output_space_size, False)
			for output in range(output_space_size):
				self.neighbor_low_masks[i][output] = not((output >> i) & 1)
				self.neighbor_high_masks[i][output] = (output >> i) & 1

		self.mask_high_shift = math.ceil(math.log2(neighbor_count))
		self.pair_masks = {}
		self.i_pair_masks = {} # inverted pair masks

	# all bits must be equal to given neighbors (AND)
	def maskForNeighborBits(self, bits: dict[int, bool]) -> BitArray:
		# assert len(bits) > 0
		# assert len([i for i in bits if i >= self.neighbor_count]) == 0
		mask = BitArray(self.output_space_size, True)
		for index, value in bits.items():
			mask &= self.neighbor_high_masks[index] if value else self.neighbor_low_masks[index]

		return mask

	def maskForPair(self, i: int, j: int) -> BitArray:
		h = (i << self.mask_high_shift) + j
		if not(h in self.pair_masks):
			self.pair_masks[h] = self.maskForNeighborBits({i: 0, j: 1})
		return self.pair_masks[h]

	def iMaskForPair(self, i: int, j: int) -> BitArray:
		h = (i << self.mask_high_shift) + j
		if not(h in self.i_pair_masks):
			self.i_pair_masks[h] = ~self.maskForPair(i, j)
		return self.i_pair_masks[h]


class OutputSpaceFactory:
	def __init__(self, neighbor_count: int, output_space_size: int, allowed_output_space: BitArray, mask_factory: NeighborMaskFactory):
		self.neighbor_count = neighbor_count
		self.output_space_size = output_space_size
		self.disallowed_output_space = ~allowed_output_space
		self.mask_factory = mask_factory

	def make(self, value: bool=True, space: BitArray=None):
		return OutputSpace(self.neighbor_count, self.output_space_size, self.mask_factory, value, space)


class OutputSpace:
	def __init__(self, neighbor_count: int, output_space_size: int, mask_factory: NeighborMaskFactory, value: bool=True, space: BitArray=None):
		self.neighbor_count = neighbor_count
		self.output_space_size = output_space_size
		self.mask_factory = mask_factory
		if space is None:
			self.space = BitArray(output_space_size, value)
		else:
			self.space = space

	def cmpSwap(self, i: int, j: int, mem: BitArray):
		# assert i < j
		# assert j < self.neighbor_count

		zeroOneMask = self.mask_factory.maskForPair(i, j)
		self.space.and_out(zeroOneMask, mem) # only select ..0..1.. indices
		mem <<= ((1 << j) - (1 << i)) # this shift is equivalent of swaping i and j for each index 
		self.space |= mem # selected ..1..0.. outputs are now possible
		self.space &= self.mask_factory.iMaskForPair(i, j) # all ..0..1.. outputs are now impossible

	def willChange(self, i: int, j: int, mem: BitArray) -> bool:
		# assert i < j
		# assert j < self.neighbor_count

		self.space.and_out(self.mask_factory.maskForPair(i, j), mem)
		return mem.fastAnyNonzero()

	def isAllowed(self, disallowed_output_space: BitArray, mem: BitArray) -> bool:
		self.space.and_out(disallowed_output_space, mem)
		return mem.fastAllZero()

	def set(self, other: OutputSpace):
		self.space.set(other.space)

	def __str__(self) -> str:
		return str(self.space)

	def hash(self) -> bytes:
		return self.space.hash()

	def prettyPrint(self):
		print("Allowed outputs:")
		n = [0] * self.neighbor_count
		for output in range(self.output_space_size):
			binaryToArray(output, n)
			if self.space[output]:
				print(f"\t{''.join(str(k) for k in n)}")
		
		print()

	def copy(self):
		return OutputSpace(self.neighbor_count, self.output_space_size, self.mask_factory, space=self.space)
		

@dataclass
class ExploredSpace:
	# when success is True, height is number of swaps to get from given state to success state
	# when success is False, height is the number of swaps tried from the given state 
	# for which none resulted in a success state
	height: int 
	swap: tuple # next swap to move towards success state
	success: bool


@dataclass
class Count:
	count: int


class NetFinder:
	def __init__(self, start_swaps: list, max_swaps: int, neighbor_count: int, allowed_output_space: BitArray):
		self.START_SWAPS = start_swaps
		self.INITIAL_SWAPS = len(start_swaps)
		self.MAX_SWAPS = max_swaps
		self.NEIGHBOR_COUNT = neighbor_count
		self.TOTAL_PAIRS = (neighbor_count - 1) * neighbor_count // 2
		self.PAIR_PERCENT = 1 / self.TOTAL_PAIRS
		self.OUTPUT_SPACE_SIZE = 2 ** neighbor_count

		self.mask_factory = NeighborMaskFactory(neighbor_count, self.OUTPUT_SPACE_SIZE)
		self.output_space_factory = OutputSpaceFactory(neighbor_count, self.OUTPUT_SPACE_SIZE, allowed_output_space, self.mask_factory)

		self.temp_mem: BitArray = None
		self.temp_spaces: list[OutputSpace] = None

	def tryFind(self):
			self.temp_mem = BitArray(self.OUTPUT_SPACE_SIZE)
			self.temp_spaces = [None] * self.MAX_SWAPS
			for i in range(self.MAX_SWAPS):
				self.temp_spaces[i] = self.output_space_factory.make()

			swaps_count = 0
			swaps = [None] * self.MAX_SWAPS
			outputSpace = self.output_space_factory.make()
			for i, j in self.START_SWAPS:
				outputSpace.cmpSwap(i, j, self.temp_mem)
				swaps[swaps_count] = (i, j)
				swaps_count += 1

			explored = {}
			best = self.findSwaps(outputSpace.copy(), swaps, swaps_count, explored, Count(0), self.MAX_SWAPS)
			while best.height != 0:
				swaps[swaps_count] = best.swap
				swaps_count += 1
				outputSpace.cmpSwap(best.swap[0], best.swap[1], BitArray(outputSpace.output_space_size))
				if (self.isAllowed(outputSpace, self.temp_mem)):
					break
				else:
					best = explored[outputSpace.hash()]
				
			return swaps[:swaps_count]

	def findSwaps(
		self,
		outputSpace: OutputSpace, 
		swaps: list[tuple[int]], 
		swaps_count: int, 
		explored_output_spaces: dict[bytes, ExploredSpace], 
		iter_count: Count,
		max_swaps: int
	) -> ExploredSpace:
		iter_count.count += 1
		if (iter_count.count >= 0b111111111111111):
			iter_count.count = 0
			percentDone = 0
			exploredCount = 0
			for k in range(self.INITIAL_SWAPS, swaps_count):
				donePairs = self.TOTAL_PAIRS - remainingPairs(swaps[k][0], swaps[k][1], self.NEIGHBOR_COUNT)
				percentDone += donePairs * (self.PAIR_PERCENT ** (k - self.INITIAL_SWAPS + 1))
				exploredCount += donePairs * nodesInTree(self.NEIGHBOR_COUNT, self.MAX_SWAPS - k - 1)

			percentDone = int(percentDone * 100 * 10000) / 10000
			print(f"Explored {exploredCount} ({percentDone}%)")
			print(f"explored_out_spaces length = {len(explored_output_spaces)}")
			print()
			
			
		if (swaps_count > max_swaps):
			return None # failed

		spaceHash = outputSpace.hash()
		if (spaceHash in explored_output_spaces):
			explored = explored_output_spaces[spaceHash]
			# fail if:
			# we find a success (which is best possible) which will take too many swaps OR
			# we find a failure which took as many (or more) swaps as we have time for, therefore we cannot find a success for this state
			failed = (explored.success and swaps_count + explored.height > max_swaps) or (not(explored.success) and explored.height >= max_swaps - swaps_count)
			if failed: return None 
			if (explored.success): return explored # successes in explored_output_spaces are always best possible, so exit immediately

		if (outputSpace.isAllowed(self.output_space_factory.disallowed_output_space, self.temp_mem)):
			return ExploredSpace(0, None, True) # success

		if (swaps_count == max_swaps):
			return None # failed

		found: ExploredSpace = None
		for i in range(NEIGHBOR_COUNT - 1):
			for j in range(i+1, NEIGHBOR_COUNT):
				mightChange = (swaps_count == 0 or not(i == swaps[swaps_count-1][0] and j == swaps[swaps_count-1][1]))
				if (mightChange and outputSpace.willChange(i, j, self.temp_mem)):
					newOut = self.temp_spaces[swaps_count]
					newOut.set(outputSpace)
					newOut.cmpSwap(i, j, self.temp_mem)
					swaps[swaps_count] = (i, j)
					maybeFound = self.findSwaps(newOut, swaps, swaps_count + 1, explored_output_spaces, iter_count, max_swaps)
					if (maybeFound is not None):
						max_swaps = swaps_count + maybeFound.height
						found = ExploredSpace(maybeFound.height + 1, (i, j), True)

		if found is not None:
			explored_output_spaces[spaceHash] = found
			return found
		elif spaceHash in explored_output_spaces:
			explored = explored_output_spaces[spaceHash]
			if (explored.height < max_swaps - swaps_count):
				explored.height = max_swaps - swaps_count
		else:
			explored_output_spaces[spaceHash] = ExploredSpace(max_swaps - swaps_count, None, False)

		return None

	def isAllowed(self, outputSpace: OutputSpace, mem: BitArray) -> bool:
		return outputSpace.isAllowed(self.output_space_factory.disallowed_output_space, mem)
