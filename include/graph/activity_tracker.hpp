#pragma once

#include "graph/csr_graph.hpp"

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace hytgraph::graph
{

    class ActivityTracker
    {
    public:
        using vertex_id = CSRGraph::vertex_id;
        using count_type = CSRGraph::offset_type;

        // A half-open vertex range [begin, end).
        //
        // This deliberately does not depend on the Phase 4 partition type.
        // Logical partitioning will own its partition representation later.
        struct VertexRange
        {
            vertex_id begin = 0;
            vertex_id end = 0;
        };

        struct PartitionActivity
        {
            count_type active_vertices = 0;
            count_type active_edges = 0;
            count_type total_edges = 0;

            [[nodiscard]] bool has_active_vertices() const noexcept
            {
                return active_vertices != 0U;
            }

            [[nodiscard]] bool has_active_edges() const noexcept
            {
                return active_edges != 0U;
            }
        };

        explicit ActivityTracker(count_type vertex_count);

        [[nodiscard]] count_type vertex_count() const noexcept;

        // Reset all vertices to inactive.
        void clear() noexcept;

        // Mark a single vertex active/inactive.
        void set_active(vertex_id vertex, bool active = true);

        [[nodiscard]] bool is_active(vertex_id vertex) const;

        // Replace the current activity state with the supplied active
        // vertex set.
        //
        // Duplicate vertex IDs are harmless.
        void set_active_vertices(const std::vector<vertex_id> &vertices);

        // Return the active vertex IDs in ascending vertex-ID order.
        [[nodiscard]] std::vector<vertex_id> active_vertices() const;

        [[nodiscard]] count_type active_vertex_count() const noexcept;

        // Count active edges according to the HyTGraph active-subgraph
        // definition: every outgoing edge of an active source vertex is
        // part of the active edge set.
        [[nodiscard]] count_type active_edge_count(
            const CSRGraph &graph) const;

        // Compute activity statistics for externally supplied vertex
        // ranges. Each range is interpreted as [begin, end).
        //
        // This keeps Phase 3 independent from the Phase 4 logical
        // partition representation.
        [[nodiscard]] std::vector<PartitionActivity>
        partition_statistics(
            const CSRGraph &graph,
            const std::vector<VertexRange> &partitions) const;

    private:
        count_type vertex_count_ = 0;
        std::vector<unsigned char> active_;
        count_type active_vertex_count_ = 0;

        void validate_vertex(vertex_id vertex) const;
        void validate_graph(const CSRGraph &graph) const;
        void validate_range(const VertexRange &range) const;
    };

} // namespace hytgraph::graph