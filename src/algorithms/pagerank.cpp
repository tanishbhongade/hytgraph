#include "algorithms/pagerank.hpp"

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

        void validate_options(
            const graph::CSRGraph &graph,
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

    } // namespace

    PageRankResult pagerank_cpu(
        const graph::CSRGraph &graph,
        const PageRankOptions &options)
    {
        validate_options(graph, options);

        const std::size_t n = static_cast<std::size_t>(graph.num_vertices());
        const float base = (1.0F - options.damping_factor) /
                           static_cast<float>(n);

        const IncomingCSR incoming = build_incoming_csr(graph);

        std::vector<float> current(n, 1.0F / static_cast<float>(n));
        std::vector<float> next(n, 0.0F);

        PageRankResult result;
        result.ranks = current;

        for (std::size_t iteration = 0; iteration < options.max_iterations;
             ++iteration)
        {
            float dangling_mass = 0.0F;
            for (graph::CSRGraph::vertex_id source = 0;
                 static_cast<graph::CSRGraph::offset_type>(source) < graph.num_vertices();
                 ++source)
            {
                if (graph.out_degree(source) == 0U)
                {
                    dangling_mass += current[static_cast<std::size_t>(source)];
                }
            }

            const float dangling_share =
                options.damping_factor * dangling_mass /
                static_cast<float>(n);

            float residual = 0.0F;
            for (std::size_t destination = 0; destination < n; ++destination)
            {
                float incoming_sum = 0.0F;
                const auto begin = incoming.offsets[destination];
                const auto end = incoming.offsets[destination + 1U];

                for (auto position = begin; position < end; ++position)
                {
                    const auto source = incoming.sources[static_cast<std::size_t>(position)];
                    const auto degree = graph.out_degree(source);
                    if (degree != 0U)
                    {
                        incoming_sum +=
                            current[static_cast<std::size_t>(source)] /
                            static_cast<float>(degree);
                    }
                }

                next[destination] = base + dangling_share +
                                    options.damping_factor * incoming_sum;
                residual += std::fabs(next[destination] - current[destination]);
            }

            current.swap(next);
            result.iterations = iteration + 1U;
            result.residual = residual;
            result.ranks = current;

            if (residual <= options.tolerance)
            {
                result.converged = true;
                return result;
            }
        }

        result.converged = false;
        return result;
    }

    std::vector<PageRankContribution> pagerank_contributions(
        const graph::CSRGraph &graph,
        const std::vector<float> &current_ranks,
        const PageRankOptions &options)
    {
        validate_options(graph, options);

        const std::size_t n =
            static_cast<std::size_t>(graph.num_vertices());

        if (current_ranks.size() != n)
        {
            throw std::invalid_argument(
                "PageRank current_ranks size must match graph vertex count");
        }

        const float base =
            (1.0F - options.damping_factor) /
            static_cast<float>(n);

        const IncomingCSR incoming = build_incoming_csr(graph);

        float dangling_mass = 0.0F;

        for (graph::CSRGraph::vertex_id source = 0;
             static_cast<graph::CSRGraph::offset_type>(source) <
             graph.num_vertices();
             ++source)
        {
            if (graph.out_degree(source) == 0U)
            {
                dangling_mass +=
                    current_ranks[static_cast<std::size_t>(source)];
            }
        }

        const float dangling_share =
            options.damping_factor * dangling_mass /
            static_cast<float>(n);

        std::vector<PageRankContribution> contributions;
        contributions.reserve(n);

        for (std::size_t destination = 0; destination < n; ++destination)
        {
            float incoming_sum = 0.0F;

            const auto begin = incoming.offsets[destination];
            const auto end = incoming.offsets[destination + 1U];

            for (auto position = begin; position < end; ++position)
            {
                const auto source =
                    incoming.sources[static_cast<std::size_t>(position)];

                const auto degree = graph.out_degree(source);

                if (degree != 0U)
                {
                    incoming_sum +=
                        current_ranks[static_cast<std::size_t>(source)] /
                        static_cast<float>(degree);
                }
            }

            const float next_rank =
                base +
                dangling_share +
                options.damping_factor * incoming_sum;

            contributions.push_back(PageRankContribution{
                destination,
                std::fabs(
                    next_rank -
                    current_ranks[destination])});
        }

        return contributions;
    }

} // namespace hytgraph::algorithms