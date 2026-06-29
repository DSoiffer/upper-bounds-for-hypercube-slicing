"""
Verify whether a set of hyperplanes slices all edges of the n-dimensional hypercube.

A hyperplane a·x + b = 0 slices an edge if its two endpoints give opposite signs
(one strictly positive, one strictly negative). Passing through a vertex does not count.
"""

from itertools import product

def slices_edge(hyperplane, v1, v2):
    """Check if hyperplane slices the edge between vertices v1 and v2.
    
    Hyperplane is [a1, a2, ..., an, b] representing a1*x1 + ... + an*xn + b = 0.
    Returns True if v1 and v2 are on strictly opposite sides.
    """
    # Compute a·v + b for each vertex
    val1 = sum(a * x for a, x in zip(hyperplane[:-1], v1)) + hyperplane[-1]
    val2 = sum(a * x for a, x in zip(hyperplane[:-1], v2)) + hyperplane[-1]
    # Sliced if one is positive and one is negative (not zero)
    return (val1 > 0 and val2 < 0) or (val1 < 0 and val2 > 0)

def get_hypercube_edges(n):
    """Generate all edges of the n-dimensional hypercube.
    
    Each edge connects two vertices differing in exactly one coordinate.
    Vertices are tuples of -1s and 1s.
    """
    edges = []
    for vertex in product([-1, 1], repeat=n):
        for i in range(n):
            if vertex[i] == -1:  # Only add edge once (when flipping -1->1)
                neighbor = tuple(1 if j == i else vertex[j] for j in range(n))
                edges.append((vertex, neighbor))
    return edges

def count_sliced_edges(hyperplanes, n):
    """Count how many edges are sliced by at least one hyperplane."""
    edges = get_hypercube_edges(n)
    sliced = 0
    for v1, v2 in edges:
        if any(slices_edge(h, v1, v2) for h in hyperplanes):
            sliced += 1
    return sliced, len(edges)

def print_slicing_results(hyperplanes):
    sliced, total = count_sliced_edges(hyperplanes, len(hyperplanes[0]) - 1)
    print(f"Sliced {sliced} out of {total} edges")
    if sliced == total:
        print("All edges sliced!")
    else:
        print(f"{total - sliced} edges remain unsliced.")


if __name__ == "__main__":
    # Hyperplane is [a1, a2, ..., an, b] representing a1*x1 + ... + an*xn + b = 0.
    
    # Example: Paterson solution, 5 hyperplanes that slice all edges of the 6-cube
    hyperplanes = [
        [ 1,  1,  1,  3,  3, -4, 0],
        [-2, -2, -2,  3,  3, -1, 0],
        [ 3,  3,  3,  1,  1, -4, 0],
        [-1, -1, -1,  3,  3,  6, 0],
        [ 3,  3,  3,  1,  1,  8, 0],
    ]
    print_slicing_results(hyperplanes)
    
    # Example: 8 hyperplanes that slice all edges of the 10-cube
    # NOTE: Bias terms are negated here from how they are presented in the paper, since
    # in the paper the rows represent a1*x1 + ... + an*xn = b. Though, strictly speaking
    # it does not make a difference for this specific example, as the fractional offsets
    # are to avoid passing exactly through vertices, and no planes pass through vertices here.
    hyperplanes = [
        [-2, -2, -2, -2, -2, -2, 1, 3, -8, -1, -0.5],
        [-2, -2, -2, -2, -2, -2, -1, -3, 8, 1, -0.5],
        [-2, -2, -2, -2, -2, -2, -1, 8, 3, -1, -0.5],
        [-2, -2, -2, -2, -2, -2, 1, -8, -3, 1, -0.5],
        [-2, -2, -2, -2, -2, -2, 4, -1, 1, -7, -0.5],
        [-2, -2, -2, -2, -2, -2, -4, 1, -1, 7, -0.5],
        [-2, -2, -2, -2, -2, -2, -7, -1, -1, -4, -0.5],
        [-2, -2, -2, -2, -2, -2, 7, 1, 1, 4, -0.5],
    ]
    print_slicing_results(hyperplanes)
