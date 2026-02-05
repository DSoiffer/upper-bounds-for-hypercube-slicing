# Rust Slicing the Hypercube (Unconstrained) OpenEvolve Configuration

This is a modified version of the Rust Adaptive Sort example from the [OpenEvolve repository](https://github.com/algorithmicsuperintelligence/openevolve) to use OpenEvolve to find an algorithm to find hyperplanes that slice all edges of a hypercube. The LLM initializes the search with a random search algorithm, and then evolves it to find a better solution. Here, we do not apply any constraints on the solutions or the search algorithm. Despite this, the LLM explores a wide range of local search strategies, including hill climbing, simulated annealing, and tabu search.

To run, clone the [OpenEvolve repository](https://github.com/algorithmicsuperintelligence/openevolve), then move this configuration into the "examples" directory, and follow the steps outlined below.

To update the problem parameters, edit the system_message in `config.yaml` and DIMENSION and NUM_PLANES in `slice_test/main.rs`.

Here are some examples of problem parameters:
- n=10, p=8, and s=5120.
- n=6, p=5, and s=192.
- n=15, p=12, and s=245760.

## Files

- `initial_program.rs`: Starting Rust implementation with random search
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

Set the `OPENAI_API_KEY` environment variable to your OpenAI-compatible API key.
```bash
export OPENAI_API_KEY=<your-api-key>
```
Update the LLM configuration in `config.yaml` for the model you are using.

## Usage

Run the evolution process:

```bash
cd examples/slicing_the_hypercube
python ../../openevolve-run.py initial_program.rs evaluator.py --config config.yaml --iterations 150

```
