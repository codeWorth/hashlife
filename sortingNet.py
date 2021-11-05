import numpy as np
import time, random, math
from collections import defaultdict

def setBitHigh(index, values):
	sectionIndex = index >> 5
	subIndex = 31 - (index & 0b11111)
	values[sectionIndex] = values[sectionIndex] | (1 << subIndex)

def setBitLow(index, values):
	sectionIndex = index >> 5
	subIndex = 31 - (index & 0b11111)
	values[sectionIndex] = values[sectionIndex] & (~(1 << subIndex))

def getBit(index, values):
	sectionIndex = index >> 5
	subIndex = 31 - (index & 0b11111)
	return (values[sectionIndex] >> subIndex) & 1

def printBits(values):
	for i in range(count):
		print(getBit(i, values), end="")

	print("")

def andAll(a, b):
	res = 0
	for i in range(a.shape[0]):
		res |= a[i] & b[i]

	return res

def toString(values):
	return ''.join(chr(x) for x in values.view(np.uint8))

sort_swaps_6 = [(1, 5),	(0, 4),	(3, 5),	(2, 4),	(1, 3),	(0, 2),	(4, 5),	(2, 3),	(0, 1),	(1, 4),	(3, 4),	(1, 2)]
astrid_swaps_8 = [
	(0, 4),	(1, 4),	(0, 1),
	(2, 5),	(3, 5),	(2, 3),
	(0, 6),	(3, 6),	(0, 3),
	(3, 7),	(4, 7),	(3, 4),
	(1, 3),	(0, 2),	(2, 3),	(0, 1),	(1, 2)
]
test_swaps_4 = [(1, 3),	(0, 2),	(2, 3),	(0, 1),	(1, 2)]
good_swaps_5 = [(0, 1), (2, 4), (0, 3), (3, 4), (1, 2), (0, 1), (1, 3)]
good_swaps_6 = [(4, 5), (1, 3), (3, 5), (1, 4), (0, 2), (2, 4), (2, 3), (0, 1), (1, 3), (1, 2)]

bits = 5
count = 1 << bits
sections = math.ceil(count/32)

def getNonBits():
	not_allowed = []
	for i in range(count):
		i_bits = [int(c) for c in f"{i:b}".zfill(bits)]

		if i_bits[bits//2-1] == 0 and (1 in i_bits[bits//2:]):
			not_allowed.append(i)
			continue

		foundZero = False
		# for n in i_bits:
		for n in i_bits[:bits//2]:
			if n == 0:
				foundZero = True
			elif n == 1 and foundZero:
				not_allowed.append(i)
				break

	notAllowedBits = np.zeros(sections, dtype=np.uint32)
	for n in not_allowed:
		setBitHigh(n, notAllowedBits)

	return notAllowedBits

notAllowedBits = getNonBits()

# lowBit must be less than highBit
def swapMutateOutput(lowBit, highBit, outputs):
	zeroBit = bits-1-lowBit
	oneBit = bits-1-highBit
	index = 0
	doSwap = (1 << zeroBit) | (1 << oneBit)

	while True:
		index += index & (1 << zeroBit)
		index += (~index) & (1 << oneBit)
		if index >= count:
			break

		if (getBit(index, outputs)):
			setBitHigh(index ^ doSwap, outputs)
			setBitLow(index, outputs)
		index += 1

combos = []
combosCount = 0
for i in range(0, bits-1):
	combos.append([])
	for j in range(i+1):
		combos[i].append(-1)
	for j in range(i+1, bits):
		combos[i].append(combosCount)
		combosCount += 1

maxDepth = 9
iterCount = 0
r = bits * (bits-1) // 2
iterTotal = (r ** (maxDepth+1) - 1) // (r - 1) - 1
def findBest(foundExact):
	global iterCount

	foundMin = defaultdict(int)
	minDepth = 0
	minStack = [(-1, None)]
	pairStack = [(0, 0)]
	outputsStack = [np.zeros(sections, dtype=np.uint32)]
	outputsStack[-1][:] = 0xFFFFFFFF
	hashStack = [toString(outputsStack[-1])]

	while True:
		i, j = pairStack[-1]
		outputs = outputsStack[-1]
		depth = len(minStack) - 1
		outputsHash = hashStack[-1]

		if not(andAll(outputs, notAllowedBits)):
			del minStack[-1]
			del pairStack[-1]
			del outputsStack[-1]
			del hashStack[-1]
			minDepth = 0
		elif depth == maxDepth:
			del minStack[-1]
			del pairStack[-1]
			del outputsStack[-1]
			del hashStack[-1]
			foundMin[outputsHash] = max(1, foundMin[outputsHash])
			minDepth = 1
		elif outputsHash in foundExact:
			del minStack[-1]
			del pairStack[-1]
			del outputsStack[-1]
			del hashStack[-1]
			minDepth = foundExact[outputsHash][0]
		elif depth+foundMin[outputsHash] > maxDepth:
			del minStack[-1]
			del pairStack[-1]
			del outputsStack[-1]
			del hashStack[-1]
			minDepth = foundMin[outputsHash]
		else:
			j += 1
			if j >= bits:
				i += 1
				j = i + 1
			pairStack[-1] = (i, j)
			
			if i >= bits-1:
				minDepth, minPair = minStack[-1]
				if depth+minDepth <= maxDepth:
					if not(outputsHash in foundExact) or minDepth < foundExact[outputsHash][0]:
						foundExact[outputsHash] = (minDepth, minPair)
				else:
					foundMin[outputsHash] = max(minDepth, foundMin[outputsHash])

				del minStack[-1]
				del pairStack[-1]
				del outputsStack[-1]
				del hashStack[-1]

				if len(minStack) == 0:
					return minDepth
			else:
				iterCount += 1
				if iterCount % 16384 == 0:
					remain = iterTotal
					for k in range(len(pairStack)):
						x, y = pairStack[k]
						pairComboCount = combos[x][y]
						remain -= pairComboCount * ((r ** (maxDepth - k) - 1) // (r - 1))

					percentDone = (1 - remain / iterTotal) * 100
					print(f"{percentDone:.2f}% done")

				newOutputs = outputs.copy()
				swapMutateOutput(i, j, newOutputs)

				minStack.append((-1, None))
				pairStack.append((0, 0))
				outputsStack.append(newOutputs)
				hashStack.append(toString(newOutputs))
				continue

		minDepth += 1
		if minDepth < minStack[-1][0] or minStack[-1][0] == -1:
			minStack[-1] = (minDepth, pairStack[-1])


foundExact = {}
minCount = findBest(foundExact)
print("min count =", minCount)
print(iterCount)
print("took", iterTotal//iterCount, "times fewer iterations w/ dynamic programming")

outputs = np.zeros(sections, dtype=np.uint32)
outputs[:] = 0xFFFFFFFF

swaps = []
for i in range(minCount):
	h = toString(outputs)
	print(foundExact[h])
	i, j = foundExact[h][1]
	swaps.append((i, j))
	swapMutateOutput(i, j, outputs)

# for i1, i2 in swaps:
# 	swapMutateOutput(i1, i2, outputs)

if andAll(outputs, notAllowedBits):
	print("FAIL")

mutateOuts = []
for i in range(count):
	if getBit(i, outputs) == 1:
		mutateOuts.append(f"{i:b}".zfill(bits))
		print(f"{i:b}".zfill(bits))

foundOuts = []
for i in range(count):
	i_bits = [int(c) for c in f"{i:b}".zfill(bits)]
	for i1, i2 in swaps:
		if i_bits[i1] == 0 and i_bits[i2] == 1:
			i_bits[i1] = 1
			i_bits[i2] = 0

	strOut = "".join([str(n) for n in i_bits])
	if not(strOut in foundOuts):
		foundOuts.append(strOut)

for found in foundOuts:
	if not(found in mutateOuts):
		print("FAIL:", found)