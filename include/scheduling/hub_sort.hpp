#pragma once

#include "graph/csr_graph.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace hytgraph::scheduling
{
    // Result of the Phase 10 hub-vertex preprocessing pass.
    //
    // The paper defines the hub importance score as:
    //
    //     H(v) = Do(v) * Di(v) / (Do_max * Di_max)
    //
    // and groups approximately the top 8% important vertices at the
    // beginning of the CSR ordering while leaving non-hub vertices in their
    // natural relative order.
    //
    // This result describes the ordering transformation without mutating the
    // existing CSRGraph. A later integration layer can apply the permutation
    // when hub preprocessing is explicitly enabled.
    struct HubSortResult
    {
        // Vertex IDs in the new logical ordering.
        //
        // The first hub_count entries are the selected hub vertices.
        // The remaining entries contain non-hub vertices in their original
        // relative order.
        std::vector<graph::CSRGraph::vertex_id> vertex_order;

        // Normalized hub importance score for every original vertex.
        // scores[v] corresponds to original vertex ID v.
        std::vector<double> scores;

        // Number of vertices selected as hubs.
        std::size_t hub_count = 0U;

        [[nodiscard]] bool empty() const noexcept
        {
            return vertex_order.empty();
        }
    };

    struct HubSortOptions
    {
        // HyTGraph paper target for the fraction of important vertices
        // grouped at the beginning of the CSR structure.
        double hub_fraction = 0.08;
    };

    // CPU/reference implementation of the paper's hub-vertex sorting
    // preprocessing.
    //
    // Responsibilities:
    //   1. compute incoming and outgoing degree for every vertex;
    //   2. compute the paper's normalized hub importance score;
    //   3. select approximately the configured top fraction of vertices;
    //   4. place selected hubs first;
    //   5. preserve the original relative order of non-hub vertices.
    //
    // This class does not:
    //   - mutate CSRGraph;
    //   - rebuild CSR storage;
    //   - execute CUDA kernels;
    //   - perform contribution-driven scheduling;
    //   - perform HyTM engine selection.
    //
    // The paper states the score and the approximately 8% grouping policy,
    // but does not specify a tie-breaking rule. The concrete implementation
    // must therefore use a deterministic engineering rule for equal scores.
    class HubSorter
    {
    public:
        explicit HubSorter(HubSortOptions options = {});

        [[nodiscard]] const HubSortOptions &
        options() const noexcept;

        // Produce the hub-first vertex ordering for the supplied CSR graph.
        //
        // The returned ordering contains every original vertex exactly once.
        // It does not modify the input graph.
        [[nodiscard]] HubSortResult
        sort(const graph::CSRGraph &graph) const;

    private:
        HubSortOptions options_;
    };

} // namespace hytgraph::scheduling