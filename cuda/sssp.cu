#include "algorithms/sssp.hpp"

#include <cuda_runtime.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace hytgraph::algorithms
{
namespace
{

void cuda_check(cudaError_t status, const char *operation)
{
    if (status != cudaSuccess)
    {
        throw std::runtime_error(
            std::string(operation) + ": " + cudaGetErrorString(status));
    }
}

struct IncomingCSR
{
    std::vector<std::uint64_t> offsets;
    std::vector<std::uint32_t> sources;
    std::vector<float> weights;
};

IncomingCSR build_incoming_csr(const graph::CSRGraph &graph)
{
    const auto vertex_count = graph.num_vertices();
    const auto edge_count = graph.num_edges();

    IncomingCSR incoming;
    incoming.offsets.assign(
        static_cast<std::size_t>(vertex_count) + 1U, 0U);
    incoming.sources.resize(static_cast<std::size_t>(edge_count));
    incoming.weights.resize(static_cast<std::size_t>(edge_count));

    for (graph::CSRGraph::vertex_id source = 0;
         static_cast<graph::CSRGraph::offset_type>(source) < vertex_count;
         ++source)
    {
        const auto [begin, end] = graph.neighbor_range(source);
        for (auto edge = begin; edge < end; ++edge)
        {
            ++incoming.offsets[
                static_cast<std::size_t>(graph.neighbor_at(edge)) + 1U];
        }
    }

    for (std::size_t vertex = 1; vertex < incoming.offsets.size(); ++vertex)
    {
        incoming.offsets[vertex] += incoming.offsets[vertex - 1U];
    }

    std::vector<std::uint64_t> cursor = incoming.offsets;
    for (graph::CSRGraph::vertex_id source = 0;
         static_cast<graph::CSRGraph::offset_type>(source) < vertex_count;
         ++source)
    {
        const auto [begin, end] = graph.neighbor_range(source);
        for (auto edge = begin; edge < end; ++edge)
        {
            const auto destination = graph.neighbor_at(edge);
            const auto position = cursor[static_cast<std::size_t>(destination)]++;
            incoming.sources[static_cast<std::size_t>(position)] = source;
            incoming.weights[static_cast<std::size_t>(position)] =
                graph.has_weights() ? graph.weight_at(edge) : 1.0F;
        }
    }

    return incoming;
}

void validate_options(const graph::CSRGraph &graph,
                      const SSSPOptions &options)
{
    if (graph.num_vertices() == 0U)
    {
        throw std::invalid_argument("SSSP requires at least one vertex");
    }
    if (static_cast<graph::CSRGraph::offset_type>(options.source) >=
        graph.num_vertices())
    {
        throw std::invalid_argument("SSSP source vertex is out of range");
    }
    if (!(options.tolerance >= 0.0F) || !std::isfinite(options.tolerance))
    {
        throw std::invalid_argument(
            "SSSP tolerance must be finite and non-negative");
    }
}

__global__ void sssp_kernel(
    const std::uint64_t *incoming_offsets,
    const std::uint32_t *incoming_sources,
    const float *incoming_weights,
    const float *current,
    const unsigned char *active,
    float *next,
    unsigned char *next_active,
    std::size_t vertex_count,
    float tolerance,
    int *changed)
{
    const std::size_t destination =
        static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (destination >= vertex_count)
    {
        return;
    }

    float best = current[destination];
    const std::uint64_t begin = incoming_offsets[destination];
    const std::uint64_t end = incoming_offsets[destination + 1U];

    for (std::uint64_t position = begin; position < end; ++position)
    {
        const std::uint32_t source = incoming_sources[position];
        if (active[source] == 0U)
        {
            continue;
        }

        const float source_distance = current[source];
        if (!isfinite(source_distance))
        {
            continue;
        }

        const float candidate = source_distance + incoming_weights[position];
        if (candidate + tolerance < best)
        {
            best = candidate;
        }
    }

    next[destination] = best;
    if (best + tolerance < current[destination])
    {
        next_active[destination] = 1U;
        atomicExch(changed, 1);
    }
    else
    {
        next_active[destination] = 0U;
    }
}

} // namespace

SSSPResult sssp_gpu(
    const graph::CSRGraph &graph,
    const SSSPOptions &options)
{
    validate_options(graph, options);

    int device_count = 0;
    cuda_check(cudaGetDeviceCount(&device_count), "cudaGetDeviceCount");
    if (device_count == 0)
    {
        throw std::runtime_error("sssp_gpu: no CUDA device is available");
    }

    const IncomingCSR incoming = build_incoming_csr(graph);
    for (const float weight : incoming.weights)
    {
        if (!std::isfinite(weight) || weight < 0.0F)
        {
            throw std::invalid_argument(
                "SSSP requires finite non-negative edge weights");
        }
    }

    const std::size_t n = static_cast<std::size_t>(graph.num_vertices());
    const std::size_t edge_count = static_cast<std::size_t>(graph.num_edges());
    const float infinity = std::numeric_limits<float>::infinity();

    std::vector<float> initial(n, infinity);
    std::vector<unsigned char> active(n, 0U);
    initial[static_cast<std::size_t>(options.source)] = 0.0F;
    active[static_cast<std::size_t>(options.source)] = 1U;

    std::size_t max_iterations = options.max_iterations;
    if (max_iterations == 0U)
    {
        max_iterations = n;
    }

    std::uint64_t *d_incoming_offsets = nullptr;
    std::uint32_t *d_incoming_sources = nullptr;
    float *d_incoming_weights = nullptr;
    float *d_current = nullptr;
    float *d_next = nullptr;
    unsigned char *d_active = nullptr;
    unsigned char *d_next_active = nullptr;
    int *d_changed = nullptr;

    try
    {
        cuda_check(cudaMalloc(&d_incoming_offsets,
                              incoming.offsets.size() * sizeof(std::uint64_t)),
                   "cudaMalloc incoming offsets");
        cuda_check(cudaMalloc(&d_incoming_sources,
                              edge_count * sizeof(std::uint32_t)),
                   "cudaMalloc incoming sources");
        cuda_check(cudaMalloc(&d_incoming_weights,
                              edge_count * sizeof(float)),
                   "cudaMalloc incoming weights");
        cuda_check(cudaMalloc(&d_current, n * sizeof(float)),
                   "cudaMalloc current distances");
        cuda_check(cudaMalloc(&d_next, n * sizeof(float)),
                   "cudaMalloc next distances");
        cuda_check(cudaMalloc(&d_active, n * sizeof(unsigned char)),
                   "cudaMalloc active flags");
        cuda_check(cudaMalloc(&d_next_active, n * sizeof(unsigned char)),
                   "cudaMalloc next active flags");
        cuda_check(cudaMalloc(&d_changed, sizeof(int)),
                   "cudaMalloc changed flag");

        cuda_check(cudaMemcpy(d_incoming_offsets, incoming.offsets.data(),
                              incoming.offsets.size() * sizeof(std::uint64_t),
                              cudaMemcpyHostToDevice),
                   "copy incoming offsets");
        cuda_check(cudaMemcpy(d_incoming_sources, incoming.sources.data(),
                              edge_count * sizeof(std::uint32_t),
                              cudaMemcpyHostToDevice),
                   "copy incoming sources");
        cuda_check(cudaMemcpy(d_incoming_weights, incoming.weights.data(),
                              edge_count * sizeof(float),
                              cudaMemcpyHostToDevice),
                   "copy incoming weights");
        cuda_check(cudaMemcpy(d_current, initial.data(), n * sizeof(float),
                              cudaMemcpyHostToDevice),
                   "copy initial distances");
        cuda_check(cudaMemcpy(d_active, active.data(), n * sizeof(unsigned char),
                              cudaMemcpyHostToDevice),
                   "copy initial active flags");

        SSSPResult result;
        result.distances = initial;

        const int block_size = 256;
        const int grid_size = static_cast<int>((n + block_size - 1U) / block_size);

        for (std::size_t iteration = 0; iteration < max_iterations; ++iteration)
        {
            cuda_check(cudaMemset(d_changed, 0, sizeof(int)),
                       "reset changed flag");
            sssp_kernel<<<grid_size, block_size>>>(
                d_incoming_offsets,
                d_incoming_sources,
                d_incoming_weights,
                d_current,
                d_active,
                d_next,
                d_next_active,
                n,
                options.tolerance,
                d_changed);
            cuda_check(cudaGetLastError(), "launch SSSP kernel");
            cuda_check(cudaDeviceSynchronize(), "synchronize SSSP kernel");

            int changed = 0;
            cuda_check(cudaMemcpy(&changed, d_changed, sizeof(int),
                                  cudaMemcpyDeviceToHost),
                       "copy SSSP changed flag");

            std::swap(d_current, d_next);
            std::swap(d_active, d_next_active);
            result.iterations = iteration + 1U;
            result.distances.resize(n);
            cuda_check(cudaMemcpy(result.distances.data(), d_current,
                                  n * sizeof(float), cudaMemcpyDeviceToHost),
                       "copy SSSP distances");

            if (changed == 0)
            {
                result.converged = true;
                cudaFree(d_incoming_offsets);
                cudaFree(d_incoming_sources);
                cudaFree(d_incoming_weights);
                cudaFree(d_current);
                cudaFree(d_next);
                cudaFree(d_active);
                cudaFree(d_next_active);
                cudaFree(d_changed);
                return result;
            }
        }

        result.converged = false;
        cudaFree(d_incoming_offsets);
        cudaFree(d_incoming_sources);
        cudaFree(d_incoming_weights);
        cudaFree(d_current);
        cudaFree(d_next);
        cudaFree(d_active);
        cudaFree(d_next_active);
        cudaFree(d_changed);
        return result;
    }
    catch (...)
    {
        cudaFree(d_incoming_offsets);
        cudaFree(d_incoming_sources);
        cudaFree(d_incoming_weights);
        cudaFree(d_current);
        cudaFree(d_next);
        cudaFree(d_active);
        cudaFree(d_next_active);
        cudaFree(d_changed);
        throw;
    }
}

} // namespace hytgraph::algorithms
