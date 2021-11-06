from __future__ import annotations
import numpy as np
import numba as nb

# short-circuiting replacement for np.any()
@nb.jit(nopython=True)
def sc_any(array):
	for x in array.flat:
		if x:
			return True
	return False


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
	def fastAnyNonzero(self) -> bool:
		return sc_any(self.chunks)

	# this will NOT handle size % self.chunk_size != 0 correctly
	def hash(self) -> bytes:
		return self.chunks.tobytes() 

	def setZeros(self):
		self.chunks[:] = 0

	def set(self, other: BitArray):
		self.chunks[:] = other.chunks[:]

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

	def and_out(self, other: BitArray, out: BitArray) -> BitArray:
		np.bitwise_and(self.chunks, other.chunks, out=out.chunks)
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
		# assert amount >= 0
		if amount >= self.size:
			self.chunks[:] = 0
			return self
		
		if (amount >= self.chunk_size):
			rolls = amount // self.chunk_size
			self.chunks[:-rolls] = self.chunks[rolls:]
			self.chunks[-rolls:] = 0
			amount -= rolls * self.chunk_size

		if amount == 0: return self

		rolled = np.empty_like(self.chunks)
		rolled[:-1] = self.chunks[1:]
		rolled[-1] = 0 
		np.left_shift(rolled, self.chunk_size - amount, out=rolled)

		np.right_shift(self.chunks, amount, out=self.chunks)
		np.bitwise_or(self.chunks, rolled, out=self.chunks)
		return self


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
		sep = " "
		out_str = sep.join(list(BitArray.CHUNK_FORMAT(self.chunk_size).format(v)[::-1] for v in self.chunks[:-1]))
		if (self.size > self.chunk_size):
			out_str += sep

		extra_bits = (self.size % self.chunk_size) or self.chunk_size
		return out_str + BitArray.CHUNK_FORMAT(extra_bits).format(self.chunks[-1])[::-1][:extra_bits]

	def __eq__(self, other: BitArray) -> bool:
		if type(other) is not BitArray: return False
		return np.array_equal(self.chunks, other.chunks)