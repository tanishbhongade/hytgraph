#include "graph/partition.hpp"

#include <stdexcept>

namespace hytgraph::graph
{

    LogicalPartition::LogicalPartition(
        vertex_id vertex_begin,
        vertex_id vertex_end,
        offset_type edge_begin,
        offset_type edge_end,
        byte_type target_bytes)
        : vertex_begin_(vertex_begin),
          vertex_end_(vertex_end),
          edge_begin_(edge_begin),
          edge_end_(edge_end),
          target_bytes_(target_bytes)
    {
        if (vertex_begin_ > vertex_end_)
        {
            throw std::invalid_argument(
                "LogicalPartition: vertex_begin must not exceed vertex_end");
        }

        if (edge_begin_ > edge_end_)
        {
            throw std::invalid_argument(
                "LogicalPartition: edge_begin must not exceed edge_end");
        }

        if (target_bytes_ == 0U)
        {
            throw std::invalid_argument(
                "LogicalPartition: target_bytes must be greater than zero");
        }
    }

    LogicalPartition::vertex_id
    LogicalPartition::vertex_begin() const noexcept
    {
        return vertex_begin_;
    }

    LogicalPartition::vertex_id
    LogicalPartition::vertex_end() const noexcept
    {
        return vertex_end_;
    }

    LogicalPartition::offset_type
    LogicalPartition::edge_begin() const noexcept
    {
        return edge_begin_;
    }

    LogicalPartition::offset_type
    LogicalPartition::edge_end() const noexcept
    {
        return edge_end_;
    }

    LogicalPartition::offset_type
    LogicalPartition::vertex_count() const noexcept
    {
        return static_cast<offset_type>(vertex_end_) -
               static_cast<offset_type>(vertex_begin_);
    }

    LogicalPartition::offset_type
    LogicalPartition::edge_count() const noexcept
    {
        return edge_end_ - edge_begin_;
    }

    LogicalPartition::byte_type
    LogicalPartition::target_bytes() const noexcept
    {
        return target_bytes_;
    }

    LogicalPartition::byte_type
    LogicalPartition::edge_data_bytes() const noexcept
    {
        return static_cast<byte_type>(edge_count()) *
               sizeof(CSRGraph::vertex_id);
    }

    LogicalPartitioner::LogicalPartitioner(byte_type partition_bytes)
        : partition_bytes_(partition_bytes)
    {
        if (partition_bytes_ == 0U)
        {
            throw std::invalid_argument(
                "LogicalPartitioner: partition_bytes must be greater than zero");
        }
    }

    LogicalPartitioner::byte_type
    LogicalPartitioner::partition_bytes() const noexcept
    {
        return partition_bytes_;
    }

    std::vector<LogicalPartition>
    LogicalPartitioner::partition(const CSRGraph &graph) const
    {
        validate_graph(graph);

        std::vector<LogicalPartition> partitions;

        const vertex_id vertex_count =
            static_cast<vertex_id>(graph.num_vertices());

        if (vertex_count == 0U)
        {
            return partitions;
        }

        vertex_id partition_begin = 0;
        offset_type partition_edge_begin = 0;

        for (vertex_id vertex = 0; vertex < vertex_count; ++vertex)
        {
            const auto [vertex_edge_begin, vertex_edge_end] =
                graph.neighbor_range(vertex);

            const offset_type candidate_edge_count =
                vertex_edge_end - partition_edge_begin;

            const byte_type candidate_edge_bytes =
                static_cast<byte_type>(candidate_edge_count) *
                sizeof(CSRGraph::vertex_id);

            /*
             * Keep every CSR row intact.
             *
             * If the current partition is already non-empty and adding
             * this vertex would exceed the requested target, close the
             * partition before this vertex.
             *
             * A single vertex whose adjacency list itself exceeds the
             * target is allowed to form an oversized partition.
             */
            if (vertex > partition_begin &&
                candidate_edge_bytes > partition_bytes_)
            {
                const auto previous_vertex_range =
                    graph.neighbor_range(vertex - 1U);

                partitions.emplace_back(
                    partition_begin,
                    vertex,
                    partition_edge_begin,
                    previous_vertex_range.second,
                    partition_bytes_);

                partition_begin = vertex;
                partition_edge_begin = vertex_edge_begin;
            }
        }

        const auto final_vertex_range =
            graph.neighbor_range(vertex_count - 1U);

        partitions.emplace_back(
            partition_begin,
            vertex_count,
            partition_edge_begin,
            final_vertex_range.second,
            partition_bytes_);

        return partitions;
    }

    void LogicalPartitioner::validate_graph(const CSRGraph &graph) const
    {
        if (graph.num_vertices() == 0U)
        {
            return;
        }

        if (graph.row_offsets().size() !=
            static_cast<std::size_t>(graph.num_vertices()) + 1U)
        {
            throw std::invalid_argument(
                "LogicalPartitioner: invalid CSR row-offset count");
        }

        if (graph.row_offsets().back() != graph.num_edges())
        {
            throw std::invalid_argument(
                "LogicalPartitioner: CSR terminal offset does not match "
                "the number of edges");
        }
    }

} // namespace hytgraph::graph