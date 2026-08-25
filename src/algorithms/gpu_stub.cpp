#include "algorithms/pagerank.hpp"
#include "algorithms/sssp.hpp"

#include <stdexcept>

namespace hytgraph::algorithms
{

    PageRankResult pagerank_gpu(
        const graph::CSRGraph &,
        const PageRankOptions &)
    {
        throw std::runtime_error(
            "pagerank_gpu: HyTGraph was built without CUDA support");
    }

    SSSPResult sssp_gpu(
        const graph::CSRGraph &,
        const SSSPOptions &)
    {
        throw std::runtime_error(
            "sssp_gpu: HyTGraph was built without CUDA support");
    }

} // namespace hytgraph::algorithms