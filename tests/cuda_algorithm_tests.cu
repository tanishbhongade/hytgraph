#include "algorithms/pagerank.hpp"
#include "algorithms/sssp.hpp"

#include <cuda_runtime.h>

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{

void expect(bool condition, const std::string &message)
{
    if (!condition)
    {
        throw std::runtime_error("FAIL: " + message);
    }
}

void test_pagerank_gpu_matches_cpu()
{
    using hytgraph::algorithms::PageRankOptions;
    using hytgraph::algorithms::pagerank_cpu;
    using hytgraph::algorithms::pagerank_gpu;
    using hytgraph::graph::CSRGraph;

    CSRGraph graph(
        3,
        {0, 2, 3, 4},
        {1, 2, 2, 0});

    PageRankOptions options;
    options.max_iterations = 200;
    options.tolerance = 1.0e-6F;

    const auto cpu = pagerank_cpu(graph, options);
    const auto gpu = pagerank_gpu(graph, options);

    expect(cpu.converged, "CPU PageRank converged");
    expect(gpu.converged, "GPU PageRank converged");
    expect(cpu.ranks.size() == gpu.ranks.size(),
           "CPU/GPU PageRank result sizes match");

    float max_error = 0.0F;
    for (std::size_t i = 0; i < cpu.ranks.size(); ++i)
    {
        max_error = std::max(max_error,
                             std::fabs(cpu.ranks[i] - gpu.ranks[i]));
    }

    expect(max_error <= 1.0e-5F,
           "GPU PageRank matches CPU reference within tolerance");
}

void test_sssp_gpu_matches_cpu()
{
    using hytgraph::algorithms::SSSPOptions;
    using hytgraph::algorithms::sssp_cpu;
    using hytgraph::algorithms::sssp_gpu;
    using hytgraph::graph::CSRGraph;

    CSRGraph graph(
        4,
        {0, 2, 4, 5, 5},
        {1, 2, 2, 3, 3},
        {1.0F, 4.0F, 2.0F, 6.0F, 1.0F});

    SSSPOptions options;
    options.source = 0;

    const auto cpu = sssp_cpu(graph, options);
    const auto gpu = sssp_gpu(graph, options);

    expect(cpu.converged, "CPU SSSP converged");
    expect(gpu.converged, "GPU SSSP converged");
    expect(cpu.distances.size() == gpu.distances.size(),
           "CPU/GPU SSSP result sizes match");

    for (std::size_t i = 0; i < cpu.distances.size(); ++i)
    {
        expect(std::fabs(cpu.distances[i] - gpu.distances[i]) <= 1.0e-5F,
               "GPU SSSP matches CPU reference within tolerance");
    }
}

} // namespace

int main()
{
    int device_count = 0;
    const auto status = cudaGetDeviceCount(&device_count);
    if (status != cudaSuccess || device_count == 0)
    {
        std::cout << "CUDA algorithm tests skipped: no CUDA device available.\n";
        return 77;
    }

    try
    {
        test_pagerank_gpu_matches_cpu();
        test_sssp_gpu_matches_cpu();
        std::cout << "CUDA algorithm tests passed.\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
