# Rust Slicing the Hypercube (Constrained) OpenEvolve Configuration

This is a modified version of the Rust Adaptive Sort example from the [OpenEvolve repository](https://github.com/algorithmicsuperintelligence/openevolve) to use OpenEvolve to find an algorithm to find hyperplanes that slice all edges of a hypercube. Here, we apply the constraint that the hyperplanes must be symmetric (following a composition, such as [6, 1, 1, 1, 1]) in the coefficients of the hyperplanes with respect to the dimensions. In this configuration, we additionally suggested more strategies for the LLM to explore, based on our previous work on this problem. The LLM initializes the search with a random search algorithm, and then evolves it to find a better solution.

To run, clone the [OpenEvolve repository](https://github.com/algorithmicsuperintelligence/openevolve), then move this configuration into the "examples" directory, and follow the steps outlined below.

To update the problem parameters, edit the system_message in `config.yaml`, DIMENSION and NUM_PLANES in `slice_test/main.rs`, and the composition (SYMMETRY) and number of allowed groups in `initial_program.rs`.

Here are some examples of problem parameters:
- n=10, p=8, and s=5120 with a [6, 1, 1, 1, 1] composition.
- n=6, p=5, and s=192 with a [3, 2, 1] composition.
- n=15, p=12, and s=245760 with a [8, 1, 1, 1, 1, 1, 1, 1] composition.

## Files

- `initial_program.rs`: Starting Rust implementation with random search, with hyperplane coefficient constraints
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
cd examples/slicing_the_hypercube_constrained
python ../../openevolve-run.py initial_program.rs evaluator.py --config config.yaml --iterations 150
```
