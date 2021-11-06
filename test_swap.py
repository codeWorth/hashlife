def CMP_SWAP(neighbors, i, j):
	temp = neighbors[i]
	neighbors[i] = (temp | neighbors[j]) & 1
	neighbors[j] = (temp & neighbors[j]) & 1

def does_conway(swaps: list[tuple]) -> bool:
	neighbors = [0, 0, 0, 0, 0, 0, 0, 0]
	for i in range(256):
		for j in range(len(neighbors)):
			neighbors[j] = (i >> j) & 1

		neighborCount = len(list(k for k in neighbors if k == 1))

		for state in range(2):
			for i, j in swaps:
				CMP_SWAP(neighbors, i, j)

			gte2_lte3 = (neighbors[0] & neighbors[1] & ~neighbors[3]) & 1
			eq3 = (neighbors[2] & ~neighbors[3]) & 1
			newState = (eq3 | (gte2_lte3 & state)) & 1

			wantedState = 0
			if (state == 1 and neighborCount >= 2 and neighborCount <= 3):
				wantedState = 1
			elif (neighborCount == 3):
				wantedState = 1

			return wantedState == newState
			# if (wantedState != newState):
			# 	newFail = [0, 0, 0, 0, 0, 0, 0, 0]
			# 	for j in range(len(newFail)):
			# 		newFail[j] = (i >> j) & 1
			# 	print("FAIL", newFail)

