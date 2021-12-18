# SIMDLife

The main program can be found in /CppHashlife

This program uses AVX-256 operations to compute the next state of Conway's Game of Life in batches of 256 cells at once. 
In each iteration, cells are put into bit arrays like so:

```
Cell state:         0 1 0 0 0 1 ...
-------------------------------------
Top left neighbor:  1 0 0 0 1 0 ...
Top neighbor:       0 1 0 0 1 0 ...
...
```

Then, each column in the above matrix is sorted using a sorting network and a compare_swap operation defined like this:
CMP_SWAP(i, j):
  matrix[i] = matrix[i] | matrix[j]
  matrix[j] = matrix[i] & matrix[j]
  
A sorting network is a fixed sequence of compare swaps which, when executed, result in a sorted array. See: https://demonstrations.wolfram.com/SortingNetworks/
For example, the fastest sorting network for an array of length 4 is:
CMP_SWAP(0, 2)
CMP_SWAP(1, 3)
CMP_SWAP(0, 1)
CMP_SWAP(2, 3)
CMP_SWAP(1, 2)

For my situation, it is not neccesary for the entire array to be completed sorted. As a result, a slightly shorter series of CMP_SWAPs can be found.
In order to find this, I wrote an optimizer, which can be found in /CppHashlife/src/find_net_v2. At it's most basic level, this optimizer tries every series of CMP_SWAPS for some max length, and returns the shortest successful one.
It eliminates many of these possible series of CMP_SWAPs using dynamic programming. After each CMP_SWAP, there is a maximum of 256 possible orderings of the length-8 array.
However, the CMP_SWAP makes some of these orderings impossible. This information is stored in a bit array which I call an output space. After each CMP_SWAP, this output space is mutated in some way.
This mutation is also done using AVX-256 operations to make it more efficient. In addition, this output space provides a convinient way to determine which results can be reused for DP purposes.
