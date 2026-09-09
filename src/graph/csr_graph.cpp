#include "graph/csr_graph.hpp"

#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace hytgraph::graph
{

    CSRGraph::CSRGraph(offset_type num_vertices,
                       std::vector<offset_type> row_offsets,
                       std::vector<vertex_id> column_indices,
                       std::vector<weight_type> edge_weights)
        : num_vertices_(num_vertices),
          row_offsets_(std::move(row_offsets)),
          column_indices_(std::move(column_indices)),
          edge_weights_(std::move(edge_weights))
    {
        validate();
    }

    CSRGraph::offset_type CSRGraph::num_vertices() const noexcept
    {
        return num_vertices_;
    }

    CSRGraph::offset_type CSRGraph::num_edges() const noexcept
    {
        return static_cast<offset_type>(column_indices_.size());
    }

    bool CSRGraph::has_weights() const noexcept
    {
        return !edge_weights_.empty();
    }

    CSRGraph::offset_type CSRGraph::out_degree(vertex_id vertex) const
    {
        const auto [begin, end] = neighbor_range(vertex);
        return end - begin;
    }

    std::pair<CSRGraph::offset_type, CSRGraph::offset_type>
    CSRGraph::neighbor_range(vertex_id vertex) const
    {
        if (static_cast<offset_type>(vertex) >= num_vertices_)
        {
            throw std::out_of_range(
                "CSRGraph::neighbor_range: vertex ID is out of range");
        }

        const offset_type begin = row_offsets_[vertex];
        const offset_type end = row_offsets_[vertex + 1];

        return {begin, end};
    }

    CSRGraph::vertex_id CSRGraph::neighbor_at(offset_type edge_index) const
    {
        if (edge_index >= num_edges())
        {
            throw std::out_of_range(
                "CSRGraph::neighbor_at: edge index is out of range");
        }

        return column_indices_[static_cast<std::size_t>(edge_index)];
    }

    CSRGraph::weight_type CSRGraph::weight_at(offset_type edge_index) const
    {
        if (!has_weights())
        {
            throw std::logic_error(
                "CSRGraph::weight_at: graph does not contain edge weights");
        }

        if (edge_index >= num_edges())
        {
            throw std::out_of_range(
                "CSRGraph::weight_at: edge index is out of range");
        }

        return edge_weights_[static_cast<std::size_t>(edge_index)];
    }

    const std::vector<CSRGraph::offset_type> &
    CSRGraph::row_offsets() const noexcept
    {
        return row_offsets_;
    }

    const std::vector<CSRGraph::vertex_id> &
    CSRGraph::column_indices() const noexcept
    {
        return column_indices_;
    }

    const std::vector<CSRGraph::weight_type> &
    CSRGraph::edge_weights() const noexcept
    {
        return edge_weights_;
    }

    void CSRGraph::validate() const
    {
        // A CSR graph needs one row-offset entry for every vertex plus
        // the terminal offset.
        const auto expected_row_offset_count =
            static_cast<std::size_t>(num_vertices_) + 1U;

        if (row_offsets_.size() != expected_row_offset_count)
        {
            throw std::invalid_argument(
                "CSRGraph::validate: row_offsets must contain num_vertices + 1 "
                "entries");
        }

        // An empty graph must still have a valid terminal offset.
        if (row_offsets_.empty())
        {
            throw std::invalid_argument(
                "CSRGraph::validate: row_offsets must not be empty");
        }

        // CSR offsets must start at zero and be monotonically non-decreasing.
        if (row_offsets_.front() != 0)
        {
            throw std::invalid_argument(
                "CSRGraph::validate: first row offset must be zero");
        }

        for (std::size_t i = 1; i < row_offsets_.size(); ++i)
        {
            if (row_offsets_[i] < row_offsets_[i - 1])
            {
                throw std::invalid_argument(
                    "CSRGraph::validate: row offsets must be non-decreasing");
            }
        }

        // The terminal row offset must identify one-past-the-last edge.
        if (row_offsets_.back() != num_edges())
        {
            throw std::invalid_argument(
                "CSRGraph::validate: terminal row offset must equal number "
                "of edges");
        }

        // Destination IDs must refer to existing vertices.
        for (const vertex_id destination : column_indices_)
        {
            if (static_cast<offset_type>(destination) >= num_vertices_)
            {
                throw std::invalid_argument(
                    "CSRGraph::validate: column index contains an invalid "
                    "destination vertex ID");
            }
        }

        // If weights are supplied, there must be exactly one weight per edge.
        if (!edge_weights_.empty() &&
            edge_weights_.size() != column_indices_.size())
        {
            throw std::invalid_argument(
                "CSRGraph::validate: edge_weights must contain exactly one "
                "weight per edge");
        }
    }

    void CSRGraph::reorder_vertices(
        const std::vector<vertex_id> &vertex_order)
    {
        const auto vertex_count =
            static_cast<std::size_t>(num_vertices_);

        if (vertex_order.size() != vertex_count)
        {
            throw std::invalid_argument(
                "CSRGraph::reorder_vertices: vertex_order must contain "
                "exactly one entry per vertex");
        }

        // vertex_order[new_vertex] = old_vertex.
        // Validate that it is a complete permutation before modifying the graph.
        std::vector<bool> seen(vertex_count, false);

        for (const vertex_id old_vertex : vertex_order)
        {
            const auto old_index =
                static_cast<std::size_t>(old_vertex);

            if (old_index >= vertex_count)
            {
                throw std::invalid_argument(
                    "CSRGraph::reorder_vertices: vertex_order contains "
                    "an out-of-range vertex ID");
            }

            if (seen[old_index])
            {
                throw std::invalid_argument(
                    "CSRGraph::reorder_vertices: vertex_order contains "
                    "duplicate vertex IDs");
            }

            seen[old_index] = true;
        }

        // Build the inverse mapping:
        // new_vertex_of_old_vertex[old_vertex] = new_vertex.
        std::vector<vertex_id> new_vertex_of_old_vertex(vertex_count);

        for (std::size_t new_vertex = 0U;
             new_vertex < vertex_count;
             ++new_vertex)
        {
            const auto old_vertex =
                static_cast<std::size_t>(vertex_order[new_vertex]);

            new_vertex_of_old_vertex[old_vertex] =
                static_cast<vertex_id>(new_vertex);
        }

        std::vector<offset_type> new_row_offsets(vertex_count + 1U, 0U);
        std::vector<vertex_id> new_column_indices;
        new_column_indices.reserve(column_indices_.size());

        std::vector<weight_type> new_edge_weights;

        if (has_weights())
        {
            new_edge_weights.reserve(edge_weights_.size());
        }

        // Copy each old vertex's adjacency list into its new vertex position.
        // Destination IDs are remapped into the new vertex numbering.
        for (std::size_t new_vertex = 0U;
             new_vertex < vertex_count;
             ++new_vertex)
        {
            const auto old_vertex = vertex_order[new_vertex];

            const auto old_vertex_index =
                static_cast<std::size_t>(old_vertex);

            const offset_type begin =
                row_offsets_[old_vertex_index];

            const offset_type end =
                row_offsets_[old_vertex_index + 1U];

            for (offset_type edge = begin;
                 edge < end;
                 ++edge)
            {
                const auto edge_index =
                    static_cast<std::size_t>(edge);

                const auto old_destination =
                    static_cast<std::size_t>(
                        column_indices_[edge_index]);

                new_column_indices.push_back(
                    new_vertex_of_old_vertex[old_destination]);

                if (has_weights())
                {
                    new_edge_weights.push_back(
                        edge_weights_[edge_index]);
                }
            }

            new_row_offsets[new_vertex + 1U] =
                static_cast<offset_type>(new_column_indices.size());
        }

        row_offsets_ = std::move(new_row_offsets);
        column_indices_ = std::move(new_column_indices);
        edge_weights_ = std::move(new_edge_weights);

        validate();
    }
} // namespace hytgraph::graph