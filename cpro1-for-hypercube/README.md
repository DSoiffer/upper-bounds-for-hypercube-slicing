# CPro1 configuration for hypercube edge slicing

Download the CPro1 code from the CPro1 repository, and follow the instructions there to set it up:
https://github.com/Constructive-Codes/CPro1

It was run using the OpenAI o4-mini model: MODEL="o4-mini-2025-04-16"

Put the following two files in the directory where you will run, replacing the versions you got from the CPro1 repository:

`conf.py` here has the setup for hypercube edge slicing.  It matches defaults, except:
- It uses a new PROMPT_NOVELTY feature.
- It generates only 200 programs per run, rather than 1000. (A total of 5 runs were done for the paper, for a total of 1000 programs.)
- The code optimization step is turned off, to save on tokens.
- It selects 15 programs for final testing against the dev set, rather than just 5.  This didn't actually seem to be necessary though.

`problem_def.py` has the problem definition.  It is configured for the small dev instances described in the paper; comment those out and uncommon the large dev instances if you want to use those.

These CPro1-produced programs are described in the paper:

`prog-cpro1-edge-weighted-search.c` has the edge-weighted search that CPro1 produced.  It is effective on the small dev instances.  After compiling, it can be run like this:

`./prog-cpro1-edge-weighted-search 7 5 440 1000 8 100 0`

This runs it on instance n=7 p=5 s=440, with random seed 1000, and hyperparameters 8 100 0 (which are the hyperparameters as tuned by CPro1).

`prog-cpro1-15-12-improvement.c` has the program that provided an improvement over our earlier results on n=15 p=12, producing a solution in which all the initial columns are the same value.  After compiling, it can be run like this:

`./prog-cpro1-15-12-improvement 15 12 245628 1000 13 20`

This runs it on instance n=15 p=12 s=245628, with random seed 1000, and hyperparameters 13 20 (which are the hyperparameters as tuned by CPro1).
