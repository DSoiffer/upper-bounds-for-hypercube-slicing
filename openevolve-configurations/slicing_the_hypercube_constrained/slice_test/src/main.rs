use slice_test::run_benchmark;

const DIMENSION: usize = 10;
const NUM_PLANES: usize = 8;

fn main() {
    let results = run_benchmark(DIMENSION, NUM_PLANES);
    
    println!("{{");
    println!("  \"uncut_edges\": {},", results.best_uncut);
    println!("  \"edges_sliced\": {},", results.edges_sliced);
    println!("  \"score\": {}", results.score);
    println!("}}");
}
