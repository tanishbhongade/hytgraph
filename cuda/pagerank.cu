#include "algorithms/pagerank.hpp"

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
            std::vector<graph::CSRGraph::offset_type> offsets;
            std::vector<graph::CSRGraph::vertex_id> sources;
        };

        IncomingCSR build_incoming_csr(const graph::CSRGraph &graph)
        {
            const auto vertex_count = graph.num_vertices();
            const auto edge_count = graph.num_edges();

            IncomingCSR incoming;
            incoming.offsets.assign(
                static_cast<std::size_t>(vertex_count) + 1U, 0U);
            incoming.sources.resize(static_cast<std::size_t>(edge_count));

            for (graph::CSRGraph::vertex_id source = 0;
                 static_cast<graph::CSRGraph::offset_type>(source) < vertex_count;
                 ++source)
            {
                const auto [begin, end] = graph.neighbor_range(source);
                for (auto edge = begin; edge < end; ++edge)
                {
                    ++incoming.offsets[static_cast<std::size_t>(graph.neighbor_at(edge)) + 1U];
                }
            }

            for (std::size_t vertex = 1; vertex < incoming.offsets.size(); ++vertex)
            {
                incoming.offsets[vertex] += incoming.offsets[vertex - 1U];
            }

            std::vector<graph::CSRGraph::offset_type> cursor = incoming.offsets;
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
                }
            }

            return incoming;
        }

        void validate_options(const graph::CSRGraph &graph,
                              const PageRankOptions &options)
        {
            if (graph.num_vertices() == 0U)
            {
                throw std::invalid_argument("PageRank requires at least one vertex");
            }
            if (!(options.damping_factor >= 0.0F && options.damping_factor < 1.0F))
            {
                throw std::invalid_argument(
                    "PageRank damping_factor must be in [0, 1)");
            }
            if (!(options.tolerance > 0.0F) || !std::isfinite(options.tolerance))
            {
                throw std::invalid_argument(
                    "PageRank tolerance must be finite and positive");
            }
            if (options.max_iterations == 0U)
            {
                throw std::invalid_argument(
                    "PageRank max_iterations must be greater than zero");
            }
        }

        __global__ void dangling_mass_kernel(
            const unsigned char *is_dangling,
            const float *ranks,
            std::size_t vertex_count,
            float *dangling_mass)
        {
            const std::size_t vertex =
                static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
            if (vertex >= vertex_count || is_dangling[vertex] == 0U)
            {
                return;
            }

            atomicAdd(dangling_mass, ranks[vertex]);
        }

        __global__ void pagerank_kernel(
            const std::uint64_t *incoming_offsets,
            const std::uint32_t *incoming_sources,
            const std::uint64_t *out_degrees,
            const float *current,
            float *next,
            std::size_t vertex_count,
            float damping_factor,
            float base,
            float dangling_share,
            float *residual)
        {
            const std::size_t vertex =
                static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
            if (vertex >= vertex_count)
            {
                return;
            }

            float incoming_sum = 0.0F;
            const std::uint64_t begin = incoming_offsets[vertex];
            const std::uint64_t end = incoming_offsets[vertex + 1U];

            for (std::uint64_t position = begin; position < end; ++position)
            {
                const std::uint32_t source = incoming_sources[position];
                const std::uint64_t degree = out_degrees[source];
                if (degree != 0U)
                {
                    incoming_sum += current[source] / static_cast<float>(degree);
                }
            }

            const float value =
                base + dangling_share + damping_factor * incoming_sum;
            next[vertex] = value;
            atomicAdd(residual, fabsf(value - current[vertex]));
        }

    } // namespace

    PageRankResult pagerank_gpu(
        const graph::CSRGraph &graph,
        const PageRankOptions &options)
    {
        validate_options(graph, options);

        int device_count = 0;
        cuda_check(cudaGetDeviceCount(&device_count), "cudaGetDeviceCount");
        if (device_count == 0)
        {
            throw std::runtime_error("pagerank_gpu: no CUDA device is available");
        }

        const std::size_t n = static_cast<std::size_t>(graph.num_vertices());
        const IncomingCSR incoming = build_incoming_csr(graph);

        std::vector<std::uint64_t> out_degrees(n);
        std::vector<unsigned char> is_dangling(n, 0U);
        for (graph::CSRGraph::vertex_id vertex = 0;
             static_cast<graph::CSRGraph::offset_type>(vertex) < graph.num_vertices();
             ++vertex)
        {
            const auto index = static_cast<std::size_t>(vertex);
            out_degrees[index] = graph.out_degree(vertex);
            is_dangling[index] = out_degrees[index] == 0U ? 1U : 0U;
        }

        std::uint64_t *d_incoming_offsets = nullptr;
        std::uint32_t *d_incoming_sources = nullptr;
        std::uint64_t *d_out_degrees = nullptr;
        unsigned char *d_is_dangling = nullptr;
        float *d_current = nullptr;
        float *d_next = nullptr;
        float *d_dangling_mass = nullptr;
        float *d_residual = nullptr;

        try
        {
            cuda_check(cudaMalloc(&d_incoming_offsets,
                                  incoming.offsets.size() * sizeof(std::uint64_t)),
                       "cudaMalloc incoming offsets");
            cuda_check(cudaMalloc(&d_incoming_sources,
                                  incoming.sources.size() * sizeof(std::uint32_t)),
                       "cudaMalloc incoming sources");
            cuda_check(cudaMalloc(&d_out_degrees,
                                  out_degrees.size() * sizeof(std::uint64_t)),
                       "cudaMalloc out degrees");
            cuda_check(cudaMalloc(&d_is_dangling,
                                  is_dangling.size() * sizeof(unsigned char)),
                       "cudaMalloc dangling flags");
            cuda_check(cudaMalloc(&d_current, n * sizeof(float)),
                       "cudaMalloc current ranks");
            cuda_check(cudaMalloc(&d_next, n * sizeof(float)),
                       "cudaMalloc next ranks");
            cuda_check(cudaMalloc(&d_dangling_mass, sizeof(float)),
                       "cudaMalloc dangling mass");
            cuda_check(cudaMalloc(&d_residual, sizeof(float)),
                       "cudaMalloc residual");

            cuda_check(cudaMemcpy(
                           d_incoming_offsets,
                           incoming.offsets.data(),
                           incoming.offsets.size() * sizeof(std::uint64_t),
                           cudaMemcpyHostToDevice),
                       "copy incoming offsets");
            cuda_check(cudaMemcpy(
                           d_incoming_sources,
                           incoming.sources.data(),
                           incoming.sources.size() * sizeof(std::uint32_t),
                           cudaMemcpyHostToDevice),
                       "copy incoming sources");
            cuda_check(cudaMemcpy(
                           d_out_degrees,
                           out_degrees.data(),
                           out_degrees.size() * sizeof(std::uint64_t),
                           cudaMemcpyHostToDevice),
                       "copy out degrees");
            cuda_check(cudaMemcpy(
                           d_is_dangling,
                           is_dangling.data(),
                           is_dangling.size() * sizeof(unsigned char),
                           cudaMemcpyHostToDevice),
                       "copy dangling flags");

            std::vector<float> initial(n, 1.0F / static_cast<float>(n));
            cuda_check(cudaMemcpy(d_current, initial.data(), n * sizeof(float),
                                  cudaMemcpyHostToDevice),
                       "copy initial ranks");

            PageRankResult result;
            result.ranks = initial;

            const int block_size = 256;
            const int grid_size = static_cast<int>((n + block_size - 1U) / block_size);
            const float base = (1.0F - options.damping_factor) /
                               static_cast<float>(n);

            for (std::size_t iteration = 0; iteration < options.max_iterations;
                 ++iteration)
            {
                cuda_check(cudaMemset(d_dangling_mass, 0, sizeof(float)),
                           "reset dangling mass");
                dangling_mass_kernel<<<grid_size, block_size>>>(
                    d_is_dangling, d_current, n, d_dangling_mass);
                cuda_check(cudaGetLastError(), "launch dangling mass kernel");
                cuda_check(cudaDeviceSynchronize(), "synchronize dangling mass kernel");

                float dangling_mass = 0.0F;
                cuda_check(cudaMemcpy(&dangling_mass, d_dangling_mass, sizeof(float),
                                      cudaMemcpyDeviceToHost),
                           "copy dangling mass");

                cuda_check(cudaMemset(d_residual, 0, sizeof(float)),
                           "reset PageRank residual");
                pagerank_kernel<<<grid_size, block_size>>>(
                    d_incoming_offsets,
                    d_incoming_sources,
                    d_out_degrees,
                    d_current,
                    d_next,
                    n,
                    options.damping_factor,
                    base,
                    options.damping_factor * dangling_mass /
                        static_cast<float>(n),
                    d_residual);
                cuda_check(cudaGetLastError(), "launch PageRank kernel");
                cuda_check(cudaDeviceSynchronize(), "synchronize PageRank kernel");

                float residual = 0.0F;
                cuda_check(cudaMemcpy(&residual, d_residual, sizeof(float),
                                      cudaMemcpyDeviceToHost),
                           "copy PageRank residual");

                std::swap(d_current, d_next);
                result.iterations = iteration + 1U;
                result.residual = residual;
                result.ranks.resize(n);
                cuda_check(cudaMemcpy(result.ranks.data(), d_current,
                                      n * sizeof(float), cudaMemcpyDeviceToHost),
                           "copy PageRank result");

                if (residual <= options.tolerance)
                {
                    result.converged = true;
                    cudaFree(d_incoming_offsets);
                    cudaFree(d_incoming_sources);
                    cudaFree(d_out_degrees);
                    cudaFree(d_is_dangling);
                    cudaFree(d_current);
                    cudaFree(d_next);
                    cudaFree(d_dangling_mass);
                    cudaFree(d_residual);
                    return result;
                }
            }

            result.converged = false;
            cudaFree(d_incoming_offsets);
            cudaFree(d_incoming_sources);
            cudaFree(d_out_degrees);
            cudaFree(d_is_dangling);
            cudaFree(d_current);
            cudaFree(d_next);
            cudaFree(d_dangling_mass);
            cudaFree(d_residual);
            return result;
        }
        catch (...)
        {
            cudaFree(d_incoming_offsets);
            cudaFree(d_incoming_sources);
            cudaFree(d_out_degrees);
            cudaFree(d_is_dangling);
            cudaFree(d_current);
            cudaFree(d_next);
            cudaFree(d_dangling_mass);
            cudaFree(d_residual);
            throw;
        }
    }

} // namespace hytgraph::algorithms
