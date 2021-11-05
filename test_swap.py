neighbors = [0, 0, 0, 0, 0, 0, 0, 0]

def CMP_SWAP(i, j):
	temp = neighbors[i]
	neighbors[i] = (temp | neighbors[j]) & 1
	neighbors[j] = (temp & neighbors[j]) & 1


for i in range(256):
	for j in range(len(neighbors)):
		neighbors[j] = (i >> j) & 1

	neighborCount = len(list(k for k in neighbors if k == 1))

	for state in range(2):
		CMP_SWAP(1, 3)
		CMP_SWAP(0, 2)
		CMP_SWAP(2, 3)
		CMP_SWAP(0, 1)
		CMP_SWAP(1, 2)
		CMP_SWAP(3, 7)
		CMP_SWAP(1, 4)
		CMP_SWAP(3, 5)
		CMP_SWAP(0, 3)
		CMP_SWAP(2, 5)
		CMP_SWAP(2, 6)
		CMP_SWAP(1, 2)
		CMP_SWAP(4, 6)
		CMP_SWAP(2, 3)
		CMP_SWAP(2, 4)
		CMP_SWAP(3, 4)

		gte2_lte3 = (neighbors[0] & neighbors[1] & ~neighbors[3]) & 1
		eq3 = (neighbors[2] & ~neighbors[3]) & 1
		newState = (eq3 | (gte2_lte3 & state)) & 1

		wantedState = 0
		if (state == 1 and neighborCount >= 2 and neighborCount <= 3):
			wantedState = 1
		elif (neighborCount == 3):
			wantedState = 1

		# print("wanted =", wantedState, "| derived =", newState)
		if (wantedState != newState):
			newFail = [0, 0, 0, 0, 0, 0, 0, 0]
			for j in range(len(newFail)):
				newFail[j] = (i >> j) & 1
			print("FAIL", newFail)

