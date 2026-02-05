// Slicing the Hypercube
// Find hyperplanes that slice all edges of a 10-dimensional hypercube

use rand::prelude::*;

// EVOLVE-BLOCK-START
// This is a random search algorithm that tries to find hyperplanes that slice all edges of a hypercube.
// This implementation can be evolved to find a better solution.
// STRATEGIES TO EXPLORE
// - Hill Climbing
// - Simulated Annealing
// - Tabu Search
// - Beam Search
// - Genetic Algorithms 
// - Edge Weighting (prioritize unsliced edges)

const SYMMETRY: &[usize] = &[6, 1, 1, 1, 1];

pub fn find_hyperplanes(dim: usize, num_planes: usize) -> Vec<Vec<i32>> {
    // ENFORCE SYMMETRY GROUP CONSTRAINT
    if SYMMETRY.len() < 5 {
        panic!("SYMMETRY must have at least 5 groups (found {})", SYMMETRY.len());
    }
    if SYMMETRY.iter().sum::<usize>() != dim {
        panic!("SYMMETRY must sum to dim ({} != {})", SYMMETRY.iter().sum::<usize>(), dim);
    }

    random_search(dim, num_planes)
}

fn random_search(dim: usize, num_planes: usize) -> Vec<Vec<i32>> {
    let mut rng = rand::rng();
    let max_coeff = 40; // Maximum coefficient value in the range [-40, 40]
    let num_tries = 100000000;
    
    let mut best_planes = Vec::new();
    let mut best_uncut = usize::MAX;
    
    for _ in 0..num_tries {
        // Generate random hyperplanes with symmetry
        let planes: Vec<Vec<i32>> = (0..num_planes)
            .map(|_| generate_symmetric_plane(&mut rng, dim, max_coeff))
            .collect();
        
        // Call helper function to count uncut edges
        let uncut = count_uncut(&planes, dim);
        
        // Update best planes if current planes have fewer uncut edges
        if uncut < best_uncut {
            best_uncut = uncut;
            best_planes = planes;
        }
        
        // If all edges are sliced, break
        if best_uncut == 0 {
            break;
        }
    }
    
    best_planes
}

// Generate a random hyperplane with symmetry
fn generate_symmetric_plane(rng: &mut impl Rng, dim: usize, max_coeff: i32) -> Vec<i32> {
    let mut plane = vec![0i32; dim + 1];
    let mut idx = 0;
    
    for &group_size in SYMMETRY {
        let coeff = rng.random_range(-max_coeff..=max_coeff);
        for _ in 0..group_size {
            if idx < dim {
                plane[idx] = coeff;
                idx += 1;
            }
        }
    }
    
    plane[dim] = rng.random_range(-max_coeff..=max_coeff);
    plane
}
// EVOLVE-BLOCK-END

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

pub fn eval_vertex(plane: &[i32], vertex_bits: usize, dim: usize) -> i32 {
    let mut sum = plane[dim];
    for d in 0..dim {
        let coord = if (vertex_bits >> d) & 1 == 1 { 1 } else { -1 };
        sum += plane[d] * coord;
    }
    sum
}

#[derive(Debug)]
pub struct BenchmarkResults {
    pub best_uncut: usize,
    pub edges_sliced: usize,
    pub score: f64,
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
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_symmetry() {
        let mut rng = rand::rng();
        let dim = 10;
        let plane = generate_symmetric_plane(&mut rng, dim, 40);
        let mut idx = 0;
        for &group_size in SYMMETRY {
            if idx < dim {
                let first_val = plane[idx];
                for _ in 0..group_size {
                    if idx < dim {
                        assert_eq!(plane[idx], first_val, "Coefficient at index {} does not match symmetry group", idx);
                        idx += 1;
                    }
                }
            }
        }
    }

    #[test]
    fn test_symmetry_group_count() {
        assert!(SYMMETRY.len() >= 5, "SYMMETRY must have at least 5 groups (found {})", SYMMETRY.len());
    }
}