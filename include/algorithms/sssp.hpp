#pragma once

#include "graph/csr_graph.hpp"

#include <cstddef>
#include <vector>

namespace hytgraph::algorithms
{

    struct SSSPOptions
    {
        graph::CSRGraph::vertex_id source = 0;
        float tolerance = 0.0F;
        std::size_t max_iterations = 0; // 0 selects |V|-1 + 1 as a safe default.
    };

    struct SSSPResult
    {
        std::vector<float> distances;
        std::size_t iterations = 0;
        bool converged = false;
    };

    SSSPResult sssp_cpu(
        const graph::CSRGraph &graph,
        const SSSPOptions &options = {});

    // GPU baseline using synchronous active-frontier relaxation.
    // When CUDA support is not built, this function throws std::runtime_error.
    SSSPResult sssp_gpu(
        const graph::CSRGraph &graph,
        const SSSPOptions &options = {});

} // namespace hytgraph::algorithms