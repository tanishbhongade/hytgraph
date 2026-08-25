#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <utility>

namespace hytgraph::graph
{

    class CSRGraph
    {
    public:
        using vertex_id = std::uint32_t;
        using offset_type = std::uint64_t;
        using weight_type = float;

        CSRGraph() = default;

        CSRGraph(offset_type num_vertices,
                 std::vector<offset_type> row_offsets,
                 std::vector<vertex_id> column_indices,
                 std::vector<weight_type> edge_weights = {});

        [[nodiscard]] offset_type num_vertices() const noexcept;
        [[nodiscard]] offset_type num_edges() const noexcept;

        [[nodiscard]] bool has_weights() const noexcept;

        // Return the number of outgoing edges of a vertex.
        [[nodiscard]] offset_type out_degree(vertex_id vertex) const;

        // Return the half-open range [begin, end) into column_indices()
        // containing the neighbors of vertex.
        [[nodiscard]] std::pair<offset_type, offset_type>
        neighbor_range(vertex_id vertex) const;

        // Return the neighbor at a particular edge position.
        [[nodiscard]] vertex_id neighbor_at(offset_type edge_index) const;

        // Return the edge weight at a particular edge position.
        [[nodiscard]] weight_type weight_at(offset_type edge_index) const;

        [[nodiscard]] const std::vector<offset_type> &row_offsets() const noexcept;
        [[nodiscard]] const std::vector<vertex_id> &column_indices() const noexcept;
        [[nodiscard]] const std::vector<weight_type> &edge_weights() const noexcept;

        // Validate all CSR invariants.
        //
        // Throws std::invalid_argument when the representation is malformed.
        void validate() const;

    private:
        offset_type num_vertices_ = 0;

        // CSR row offsets. Expected size is num_vertices_ + 1.
        std::vector<offset_type> row_offsets_;

        // Destination vertex ID for every edge.
        std::vector<vertex_id> column_indices_;

        // Optional edge weights. Empty means the graph is unweighted.
        std::vector<weight_type> edge_weights_;
    };

} // namespace hytgraph::graph