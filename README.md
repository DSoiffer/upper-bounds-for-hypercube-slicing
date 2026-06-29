# Improved Upper Bounds for Slicing the Hypercube
Accompanying code and results for the paper "Improved Upper Bounds for Slicing the Hypercube"

The file `verify_slicing.py` contains a simple, commented Python script that can be used to verify the correctness of the constructions presented in the paper.


### The Search Algorithm
The file `edgeweighted_search.c` contains the final primary search algorithm we use, along with comments describing its functionality.
To compile and run, you will need a C compiler. Compile it with

`gcc -march=native -O3 -o edgeweighted_search edgeweighted_search.c -lm`

and then run it like this:

`./edgeweighted_search 10 8 5120 0 200 1 200000 1000`

The "10 8 5120" specifies dimension $n$ = 10, number of planes $k$ = 8, and slicing at least 5120 edges (full slicing in this case). It will keep running until it has found a collection of planes slicing at least that many edges, printing the best results it has found so far as it proceeds.
The "0" is the random seed.

The remaining arguments are hyperparameters which we recommend specifying:
- weight_period: iterations before boosting unsliced edge weights, we recommend 200 for $n$ < 11, and 5000 otherwise
- weight_increment: amount to increase weights, we recommend 1
- restart_period: iterations before random restart, we recommend 200000 for $n$ < 11, and 5000000 otherwise
- weight_cap: maximum weight value, we recommend 1000


### CPro1, OpenEvolve Configurations
The configurations we used for CPro1 and OpenEvolve are given, as well as additional instructions, in their respective directories.

### List of Constructions
The collections of planes accompanying every result given in the paper are listed in `results.txt`, format: `n k [number of edges sliced]` followed by the collection of $k$ planes which slices that many edges of the $n$-dimensional hypercube.

Additional constructions are presented in `other_constructions`, including other full constructions, other partial constructions for values of $n, k$ where no full solution is known, and partial constructions which slice many edges in the reduced hypercube but which may not be the best partial solution in the unreduced hypercube. Files are organized by hypercube dimension $n$ and the composition used for the reduced hypercube.

### Solution Verification
In addition to the code here, an interactive website for inspecting and verifying solutions more closely is provided at [hypercube-slicing.pages.dev](https://hypercube-slicing.pages.dev).
