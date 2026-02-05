# Rust Slicing the Hypercube (Constrained) OpenEvolve Configuration

This is a modified version of the Rust Adaptive Sort example from the [OpenEvolve repository](https://github.com/algorithmicsuperintelligence/openevolve) to use OpenEvolve to find an algorithm to find hyperplanes that slice all edges of a hypercube. Here, we apply the constraint that the hyperplanes must be symmetric (following a composition, such as [6, 1, 1, 1, 1]) in the coefficients of the hyperplanes with respect to the dimensions. In this configuration, we additionally suggested more strategies for the LLM to explore, based on our previous work on this problem. The LLM initializes the search with a random search algorithm, and then evolves it to find a better solution.

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
cd examples/slicing_the_hypercube_constrained
python ../../openevolve-run.py initial_program.rs evaluator.py --config config.yaml --iterations 150
```
