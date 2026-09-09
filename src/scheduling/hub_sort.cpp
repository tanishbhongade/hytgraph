#include "scheduling/hub_sort.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace hytgraph::scheduling
{
    HubSorter::HubSorter(HubSortOptions options)
        : options_(options)
    {
        if (!std::isfinite(options_.hub_fraction) ||
            options_.hub_fraction < 0.0 ||
            options_.hub_fraction > 1.0)
        {
            throw std::invalid_argument(
                "HubSortOptions.hub_fraction must be finite and in [0, 1]");
        }
    }

    const HubSortOptions &
    HubSorter::options() const noexcept
    {
        return options_;
    }

    HubSortResult
    HubSorter::sort(const graph::CSRGraph &graph) const
    {
        const std::size_t vertex_count =
            static_cast<std::size_t>(graph.num_vertices());

        HubSortResult result;
        result.vertex_order.reserve(vertex_count);
        result.scores.resize(vertex_count, 0.0);

        if (vertex_count == 0U)
        {
            return result;
        }

        // Compute in-degree from the CSR destination array.
        // Out-degree is already represented by each CSR row length.
        std::vector<std::size_t> in_degree(vertex_count, 0U);

        for (std::size_t edge = 0U;
             edge < static_cast<std::size_t>(graph.num_edges());
             ++edge)
        {
            const auto destination =
                graph.neighbor_at(
                    static_cast<graph::CSRGraph::offset_type>(edge));

            ++in_degree[static_cast<std::size_t>(destination)];
        }

        std::size_t max_out_degree = 0U;
        std::size_t max_in_degree = 0U;

        for (std::size_t vertex = 0U;
             vertex < vertex_count;
             ++vertex)
        {
            const std::size_t out_degree =
                static_cast<std::size_t>(
                    graph.out_degree(
                        static_cast<graph::CSRGraph::vertex_id>(vertex)));

            max_out_degree = std::max(max_out_degree, out_degree);
            max_in_degree = std::max(max_in_degree, in_degree[vertex]);
        }

        // Paper definition:
        //
        //   H(v) = Do(v) * Di(v) / (Do_max * Di_max)
        //
        // For an edgeless graph the denominator is zero. The paper does not
        // specify this edge case, so the reference implementation assigns
        // zero to every score.
        const long double denominator =
            static_cast<long double>(max_out_degree) *
            static_cast<long double>(max_in_degree);

        if (denominator > 0.0L)
        {
            for (std::size_t vertex = 0U;
                 vertex < vertex_count;
                 ++vertex)
            {
                const long double numerator =
                    static_cast<long double>(
                        graph.out_degree(
                            static_cast<graph::CSRGraph::vertex_id>(vertex))) *
                    static_cast<long double>(in_degree[vertex]);

                result.scores[vertex] =
                    static_cast<double>(numerator / denominator);
            }
        }

        // The paper specifies approximately the top 8%, but does not state
        // the integer rounding rule. We use ceil() so a positive configured
        // fraction does not silently select zero hubs on small graphs.
        const long double requested_hubs =
            static_cast<long double>(vertex_count) *
            static_cast<long double>(options_.hub_fraction);

        const long double rounded_requested_hubs =
            std::round(requested_hubs);

        const std::size_t hub_count =
            std::min(
                vertex_count,
                static_cast<std::size_t>(
                    std::fabs(requested_hubs - rounded_requested_hubs) < 1e-12L
                        ? rounded_requested_hubs
                        : std::ceil(requested_hubs)));

        result.hub_count = hub_count;

        std::vector<graph::CSRGraph::vertex_id> ranked_vertices;
        ranked_vertices.reserve(vertex_count);

        for (std::size_t vertex = 0U;
             vertex < vertex_count;
             ++vertex)
        {
            ranked_vertices.push_back(
                static_cast<graph::CSRGraph::vertex_id>(vertex));
        }

        // The paper does not specify tie-breaking.
        // Deterministic engineering rule:
        //   1. higher hub score first;
        //   2. lower original vertex ID first on equal scores.
        std::stable_sort(
            ranked_vertices.begin(),
            ranked_vertices.end(),
            [&result](
                const graph::CSRGraph::vertex_id lhs,
                const graph::CSRGraph::vertex_id rhs)
            {
                const double lhs_score =
                    result.scores[static_cast<std::size_t>(lhs)];

                const double rhs_score =
                    result.scores[static_cast<std::size_t>(rhs)];

                if (lhs_score != rhs_score)
                {
                    return lhs_score > rhs_score;
                }

                return lhs < rhs;
            });

        std::vector<bool> is_hub(vertex_count, false);

        for (std::size_t rank = 0U;
             rank < hub_count;
             ++rank)
        {
            const auto vertex = ranked_vertices[rank];

            is_hub[static_cast<std::size_t>(vertex)] = true;
            result.vertex_order.push_back(vertex);
        }

        // Non-hub vertices retain their natural relative order.
        for (std::size_t vertex = 0U;
             vertex < vertex_count;
             ++vertex)
        {
            if (!is_hub[vertex])
            {
                result.vertex_order.push_back(
                    static_cast<graph::CSRGraph::vertex_id>(vertex));
            }
        }

        return result;
    }

} // namespace hytgraph::scheduling