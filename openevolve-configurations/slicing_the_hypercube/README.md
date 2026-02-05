# Rust Slicing the Hypercube (Unconstrained) OpenEvolve Configuration

This is a modified version of the Rust Adaptive Sort example from the [OpenEvolve repository](https://github.com/algorithmicsuperintelligence/openevolve) to use OpenEvolve to find an algorithm to find hyperplanes that slice all edges of a hypercube. The LLM initializes the search with a random search algorithm, and then evolves it to find a better solution. Here, we do not apply any constraints on the solutions or the search algorithm. Despite this, the LLM explores a wide range of local search strategies, including hill climbing, simulated annealing, and tabu search.

To run, clone the [OpenEvolve repository](https://github.com/algorithmicsuperintelligence/openevolve), then move this configuration into the "examples" directory, and follow the steps outlined below.

## Files

- `initial_program.rs`: Starting Rust implementation with basic quicksort
- `evaluator.py`: Python evaluator that compiles and benchmarks Rust code
- `config.yaml`: Configuration optimized for performance-critical algorithm evolution. This specifies the prompt for the LLM.
- `requirements.txt`: System dependencies and Python requirements

## Prerequisites

### System Dependencies

1. **Rust Toolchain**: Install from [rustup.rs](https://rustup.rs/)

   ```bash
   curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
   source ~/.cargo/env
   ```

2. **Cargo**: Comes with Rust installation

### Python Dependencies

```bash
pip install -r requirements.txt
```

### API KEY

You will need to set the `OPENAI_API_KEY` environment variable to your OpenAI API key.

```bash
export OPENAI_API_KEY=<your-api-key>
```

## Usage

Run the evolution process:

```bash
cd examples/slicing_the_hypercube
python ../../openevolve-run.py initial_program.rs evaluator.py --config config.yaml --iterations 150

```
