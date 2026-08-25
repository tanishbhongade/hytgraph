#include "algorithms/sssp.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

namespace hytgraph::algorithms
{
    namespace
    {

        struct IncomingCSR
        {
            std::vector<graph::CSRGraph::offset_type> offsets;
            std::vector<graph::CSRGraph::vertex_id> sources;
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
                    const auto destination = graph.neighbor_at(edge);
                    ++incoming.offsets[static_cast<std::size_t>(destination) + 1U];
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
                    incoming.weights[static_cast<std::size_t>(position)] =
                        graph.has_weights() ? graph.weight_at(edge) : 1.0F;
                }
            }

            return incoming;
        }

        void validate_graph(const graph::CSRGraph &graph,
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

    } // namespace

    SSSPResult sssp_cpu(
        const graph::CSRGraph &graph,
        const SSSPOptions &options)
    {
        validate_graph(graph, options);

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
        const float infinity = std::numeric_limits<float>::infinity();

        std::vector<float> current(n, infinity);
        std::vector<float> next = current;
        std::vector<unsigned char> active(n, 0U);
        std::vector<unsigned char> next_active(n, 0U);

        current[static_cast<std::size_t>(options.source)] = 0.0F;
        active[static_cast<std::size_t>(options.source)] = 1U;

        std::size_t max_iterations = options.max_iterations;
        if (max_iterations == 0U)
        {
            max_iterations = n;
        }

        SSSPResult result;
        result.distances = current;

        for (std::size_t iteration = 0; iteration < max_iterations; ++iteration)
        {
            next = current;
            std::fill(next_active.begin(), next_active.end(), 0U);
            bool changed = false;

            for (std::size_t destination = 0; destination < n; ++destination)
            {
                float best = current[destination];
                const auto begin = incoming.offsets[destination];
                const auto end = incoming.offsets[destination + 1U];

                for (auto position = begin; position < end; ++position)
                {
                    const auto source = incoming.sources[static_cast<std::size_t>(position)];
                    if (active[static_cast<std::size_t>(source)] == 0U)
                    {
                        continue;
                    }

                    const float source_distance = current[static_cast<std::size_t>(source)];
                    if (!std::isfinite(source_distance))
                    {
                        continue;
                    }

                    const float candidate =
                        source_distance + incoming.weights[static_cast<std::size_t>(position)];
                    if (candidate + options.tolerance < best)
                    {
                        best = candidate;
                    }
                }

                next[destination] = best;
                if (best + options.tolerance < current[destination])
                {
                    next_active[destination] = 1U;
                    changed = true;
                }
            }

            current.swap(next);
            active.swap(next_active);
            result.iterations = iteration + 1U;
            result.distances = current;

            if (!changed)
            {
                result.converged = true;
                return result;
            }
        }

        result.converged = false;
        return result;
    }

} // namespace hytgraph::algorithms