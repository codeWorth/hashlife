import cv2, random, time
import numpy as np

cellSize = 3
cellChance = 4 # 1 in cellChance for alive cell
depth = 8
board = np.empty((2**(depth), 2**(depth)), dtype=np.uint8)
img = np.zeros((board.shape[0]*cellSize, board.shape[1]*cellSize), dtype=np.uint8)

class Leaf:
	def makeLeaf(state):
		return Node.NODE_CACHE[state]

	def __init__(self, state):
		self.state = state
		self.depth = 0

	def __eq__(self, other):
		return id(self) == id(other)

	def __str__(self):
		return str(self.state)

	def writeToBoard(self, board):
		board[0,0] = self.state

	def pruneEqual(self):
		pass

	def countUnique(self):
		return 1

	def nextGen(self, *neighborhood):
		aliveNeighbors = sum(cell.state for cell in neighborhood)
		nextState = 0
		if (self.state == 1 and aliveNeighbors == 2) or aliveNeighbors == 3:
			nextState = 1

		return Leaf.makeLeaf(nextState)

class Node:
	NODE_CACHE = {0: Leaf(0), 1: Leaf(1)}
	GEN_CACHE = {}

	def makeNode(nw, ne, sw, se):
		node_hash = (id(nw), id(ne), id(sw), id(se))
		if not(node_hash in Node.NODE_CACHE):
			Node.NODE_CACHE[node_hash] = Node(nw.depth+1, nw, ne, sw, se)

		return Node.NODE_CACHE[node_hash]

	def makeZero(depth):
		if depth == 0:
			return Leaf.makeLeaf(0)
		else:
			z = Node.makeZero(depth-1)
			return Node.makeNode(z, z, z, z)

	def __init__(self, depth, nw, ne, sw, se):
		self.depth = depth
		self.nw = nw
		self.ne = ne
		self.sw = sw
		self.se = se

	def __eq__(self, other):
		return id(self) == id(other)

	def __str__(self):
		if self.depth == 1:
			return f"nw {str(self.nw)}\nne {str(self.ne)}\nsw {str(self.sw)}\nse {str(self.se)}\n"
		else:	
			nwStr = str(self.nw).split("\n")
			neStr = str(self.ne).split("\n")
			swStr = str(self.sw).split("\n")
			seStr = str(self.se).split("\n")

			outStr = "nw {\n"
			for s in nwStr:
				outStr += "\t" + s + "\n"

			outStr += "}\nne {\n"
			for s in neStr:
				outStr += "\t" + s + "\n"

			outStr += "}\nsw {\n"
			for s in swStr:
				outStr += "\t" + s + "\n"

			outStr += "}\nse {\n"
			for s in seStr:
				outStr += "\t" + s + "\n"

			outStr += "}"
			return outStr

	def writeToBoard(self, board):
		size = board.shape[0]
		self.nw.writeToBoard(board[:size//2, :size//2])
		self.ne.writeToBoard(board[size//2:, :size//2])
		self.sw.writeToBoard(board[:size//2, size//2:])
		self.se.writeToBoard(board[size//2:, size//2:])

	def countUnique(self):
		unique = self.se.countUnique()
		if not(self.nw is self.ne or self.nw is self.sw or self.nw is self.se):
			unique += self.nw.countUnique()
		if not(self.ne is self.sw or self.ne is self.se):
			unique += self.ne.countUnique()
		if not(self.sw is self.se):
			unique += self.sw.countUnique()

		return unique

	def life4x4(self):
		nwNext = self.nw.se.nextGen(
			self.nw.nw, self.nw.ne, self.ne.nw, 
			self.nw.sw,             self.ne.sw,
			self.sw.nw, self.sw.ne, self.se.nw)
		neNext = self.ne.sw.nextGen(
			self.nw.ne, self.ne.nw, self.ne.ne, 
			self.nw.se,             self.ne.se,
			self.sw.ne, self.se.nw, self.se.ne)
		swNext = self.sw.ne.nextGen(
			self.nw.sw, self.nw.se, self.ne.sw, 
			self.sw.nw,             self.se.nw,
			self.sw.sw, self.sw.se, self.se.sw)
		seNext = self.se.nw.nextGen(
			self.nw.se, self.ne.sw, self.ne.se, 
			self.sw.ne,             self.se.ne,
			self.sw.se, self.se.sw, self.se.se)

		return Node.makeNode(nwNext, neNext, swNext, seNext)

	def nextGen(self):
		if not id(self) in Node.GEN_CACHE:
			if self.depth == 2:
				Node.GEN_CACHE[id(self)] = self.life4x4()
			else:
				c1 = self.nw.nextGen()
				c2 = Node.makeNode(self.nw.ne, self.ne.nw, self.nw.se, self.ne.sw).nextGen()
				c3 = self.ne.nextGen()
				c4 = Node.makeNode(self.nw.sw, self.nw.se, self.sw.nw, self.sw.ne).nextGen()
				c5 = Node.makeNode(self.nw.se, self.ne.sw, self.sw.ne, self.se.nw).nextGen()
				c6 = Node.makeNode(self.ne.sw, self.ne.se, self.se.nw, self.se.ne).nextGen()
				c7 = self.sw.nextGen()
				c8 = Node.makeNode(self.sw.ne, self.se.nw, self.sw.se, self.se.sw).nextGen()
				c9 = self.se.nextGen()

				nw = Node.makeNode(c1.se, c2.sw, c4.ne, c5.nw)
				ne = Node.makeNode(c2.se, c3.sw, c5.ne, c6.nw)
				sw = Node.makeNode(c4.se, c5.sw, c7.ne, c8.nw)
				se = Node.makeNode(c5.se, c6.sw, c8.ne, c9.nw)
				# nw = Node.makeNode(c1, c2, c4, c5).nextGen()
				# ne = Node.makeNode(c2, c3, c5, c6).nextGen()
				# sw = Node.makeNode(c4, c5, c7, c8).nextGen()
				# se = Node.makeNode(c5, c6, c8, c9).nextGen()
				Node.GEN_CACHE[id(self)] = Node.makeNode(nw, ne, sw, se)

		return Node.GEN_CACHE[id(self)]

def makeNode(deep):
	if deep == 0:
		return Leaf.makeLeaf(random.randint(0, cellChance)//cellChance)
	else:
		return Node.makeNode(
			makeNode(deep-1),
			makeNode(deep-1),
			makeNode(deep-1),
			makeNode(deep-1))

def drawBoard(board, img, cellSize, name="board"):
	for i in range(cellSize):
		for j in range(cellSize):
			img[i::cellSize,j::cellSize] = board*220

	# for i in range(board.shape[0]):
	# 	img[i*cellSize,:] = 255
	# 	img[:,i*cellSize] = 255

	cv2.imshow(name, img)

zN = Node.makeZero(depth-1)
def center(node):
	nw = Node.makeNode(zN, zN, zN, node.nw)
	ne = Node.makeNode(zN, zN, node.ne, zN)
	sw = Node.makeNode(zN, node.sw, zN, zN)
	se = Node.makeNode(node.se, zN, zN, zN)
	return Node.makeNode(nw, ne, sw, se)

def _setCell(node, bI, bJ, state):
	if len(bI) == 0:
		return Leaf.makeLeaf(state)

	i = bI[0]
	j = bJ[0]
	if i == '0' and j == '0':
		subCell = _setCell(node.nw, bI[1:], bJ[1:], state)
		return Node.makeNode(subCell, node.ne, node.sw, node.se)
	elif i == '1' and j == '0':
		subCell = _setCell(node.ne, bI[1:], bJ[1:], state)
		return Node.makeNode(node.nw, subCell, node.sw, node.se)
	elif i == '0' and j == '1':
		subCell = _setCell(node.sw, bI[1:], bJ[1:], state)
		return Node.makeNode(node.nw, node.ne, subCell, node.se)
	elif i == '1' and j == '1':
		subCell = _setCell(node.se, bI[1:], bJ[1:], state)
		return Node.makeNode(node.nw, node.ne, node.sw, subCell)

def setCell(node, i, j, state):
	bI = "{0:b}".format(i).zfill(node.depth)
	bJ = "{0:b}".format(j).zfill(node.depth)
	return _setCell(node, bI, bJ, state)

node = makeNode(depth)

frameCount = 0
iterCount = 0
while cv2.waitKey(1) & 0xFF != ord('q'):
	t1 = time.time_ns()
	node.writeToBoard(board)
	t2 = time.time_ns()
	drawBoard(board, img, cellSize)
	t3 = time.time_ns()
	node = center(node).nextGen()
	iterCount += 2**(node.depth-1)
	t4 = time.time_ns()

	print("write =", (t2 - t1))
	print("draw =", (t3 - t2))
	print("calc =", (t4 - t3))

	frameCount += 1
	if frameCount % 20 == 0:
		print("{:.3f} mb".format((len(Node.NODE_CACHE)*4*8 + len(Node.GEN_CACHE)*4*2)/1000000))