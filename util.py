NEIGHBOR_COUNT = 8

def nodesInTree(k: int, h: int) -> int:
	return (k ** (h + 1) - 1) // (k - 1)

# including the given pair
def remainingPairs(i: int, j: int, neighbor_count: int) -> int:
	iLeft = neighbor_count - i - 1
	return iLeft * (iLeft - 1) // 2 + neighbor_count - j

def binaryToArray(value: int, out_array: list):
    for i in range(len(out_array)):
        out_array[i] = (value >> i) & 1