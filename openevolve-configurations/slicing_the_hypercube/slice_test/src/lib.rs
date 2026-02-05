// Find hyperplanes that slice all edges of a 10-dimensional hypercube

use rand::prelude::*;

// EVOLVE-BLOCK-START
// This is a random search algorithm that tries to find hyperplanes that slice all edges of a hypercube.
// This implementation can be evolved to find a better solution.
pub fn find_hyperplanes(dim: usize, num_planes: usize) -> Vec<Vec<i32>> {
    random_search(dim, num_planes)
}

fn random_search(dim: usize, num_planes: usize) -> Vec<Vec<i32>> {
    let mut rng = rand::rng();
    let max_coeff = 40; // Maximum coefficient value in the range [-40, 40]
    let num_tries = 1000000;
    
    let mut best_planes = Vec::new();
    let mut best_uncut = usize::MAX;
    
    for _ in 0..num_tries {
        // Generate random hyperplanes
        let planes: Vec<Vec<i32>> = (0..num_planes)
            .map(|_| generate_random_plane(&mut rng, dim, max_coeff))
            .collect();
        
        // Call helper function to count uncut edges
        let uncut = count_uncut(&planes, dim);
        
        // Update best planes if current planes are better
        if uncut < best_uncut {
            best_uncut = uncut;
            best_planes = planes;
        }
        
        // If we have found a solution that slices all edges
        if best_uncut == 0 {
            break;
        }
    }
    
    best_planes
}

// Helper function to generate random hyperplanes
fn generate_random_plane(rng: &mut impl Rng, dim: usize, max_coeff: i32) -> Vec<i32> {
    let mut plane = vec![0i32; dim + 1];
    for i in 0..=dim {
        plane[i] = rng.random_range(-max_coeff..=max_coeff);
    }
    plane
}

// Helper function to count uncut edges
pub fn count_uncut(planes: &[Vec<i32>], dim: usize) -> usize {
    let num_vertices = 1 << dim;
    let mut uncut = 0;
    
    for i in 0..num_vertices {
        for bit in 0..dim {
            let j = i ^ (1 << bit);
            if i < j {
                let mut sliced = false;
                for plane in planes {
                    let sign_i = eval_vertex(plane, i, dim);
                    let sign_j = eval_vertex(plane, j, dim);
                    if (sign_i > 0) != (sign_j > 0) {
                        sliced = true;
                        break;
                    }
                }
                if !sliced {
                    uncut += 1;
                }
            }
        }
    }
    uncut
}

// Helper function to evaluate a plane at a vertex
pub fn eval_vertex(plane: &[i32], vertex_bits: usize, dim: usize) -> i32 {
    let mut sum = plane[dim];
    for d in 0..dim {
        let coord = if (vertex_bits >> d) & 1 == 1 { 1 } else { -1 };
        sum += plane[d] * coord;
    }
    sum
}
// EVOLVE-BLOCK-END

#[derive(Debug)]
pub struct BenchmarkResults {
    pub best_uncut: usize,
    pub edges_sliced: usize,
    pub score: f64,
    pub best_planes: Vec<Vec<i32>>,
}

pub fn run_benchmark(dim: usize, num_planes: usize) -> BenchmarkResults {
    let planes = find_hyperplanes(dim, num_planes);
    
    let total_edges = dim * (1 << (dim - 1));
    let uncut = count_uncut(&planes, dim);
    
    let edges_sliced = total_edges - uncut;
    let score = edges_sliced as f64 / total_edges as f64;
    
    BenchmarkResults {
        best_uncut: uncut,
        edges_sliced,
        score,
        best_planes: planes,
    }
}

#[cfg(test)]
mod tests {
    use super::*;
}