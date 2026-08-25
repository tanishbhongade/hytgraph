#pragma once

#include "graph/csr_graph.hpp"

#include <cstddef>
#include <vector>

namespace hytgraph::algorithms
{

    struct PageRankOptions
    {
        float damping_factor = 0.85F;
        float tolerance = 1.0e-6F;
        std::size_t max_iterations = 100;
    };

    struct PageRankResult
    {
        std::vector<float> ranks;
        std::size_t iterations = 0;
        float residual = 0.0F;
        bool converged = false;
    };

    PageRankResult pagerank_cpu(
        const graph::CSRGraph &graph,
        const PageRankOptions &options = {});

    // GPU baseline using the same synchronous update semantics as pagerank_cpu.
    // When CUDA support is not built, this function throws std::runtime_error.
    PageRankResult pagerank_gpu(
        const graph::CSRGraph &graph,
        const PageRankOptions &options = {});

} // namespace hytgraph::algorithms