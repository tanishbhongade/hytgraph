#pragma once

#include "graph/csr_graph.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace hytgraph::graph
{

    class LogicalPartition
    {
    public:
        using vertex_id = CSRGraph::vertex_id;
        using offset_type = CSRGraph::offset_type;
        using byte_type = std::size_t;

        LogicalPartition() = default;

        LogicalPartition(vertex_id vertex_begin,
                         vertex_id vertex_end,
                         offset_type edge_begin,
                         offset_type edge_end,
                         byte_type target_bytes);

        [[nodiscard]] vertex_id vertex_begin() const noexcept;
        [[nodiscard]] vertex_id vertex_end() const noexcept;

        [[nodiscard]] offset_type edge_begin() const noexcept;
        [[nodiscard]] offset_type edge_end() const noexcept;

        [[nodiscard]] offset_type vertex_count() const noexcept;
        [[nodiscard]] offset_type edge_count() const noexcept;

        [[nodiscard]] byte_type target_bytes() const noexcept;

        // Returns the number of bytes occupied by the CSR edge
        // destinations represented by this partition.
        //
        // This intentionally counts only the destination array here.
        // Vertex values, row offsets, and other runtime metadata are
        // separate from the logical edge partition.
        [[nodiscard]] byte_type edge_data_bytes() const noexcept;

    private:
        vertex_id vertex_begin_ = 0;
        vertex_id vertex_end_ = 0;

        offset_type edge_begin_ = 0;
        offset_type edge_end_ = 0;

        byte_type target_bytes_ = 0;
    };

    class LogicalPartitioner
    {
    public:
        using vertex_id = CSRGraph::vertex_id;
        using offset_type = CSRGraph::offset_type;
        using byte_type = std::size_t;

        // HyTGraph uses 32 MB logical partitions for fine-grained
        // cost analysis.
        static constexpr byte_type kDefaultPartitionBytes =
            32ULL * 1024ULL * 1024ULL;

        explicit LogicalPartitioner(
            byte_type partition_bytes = kDefaultPartitionBytes);

        [[nodiscard]] byte_type partition_bytes() const noexcept;

        // Partition the CSR graph into contiguous vertex ranges.
        //
        // Partitions never split a vertex's adjacency list. The requested
        // byte size is therefore a target rather than an exact size.
        [[nodiscard]] std::vector<LogicalPartition>
        partition(const CSRGraph &graph) const;

    private:
        byte_type partition_bytes_;

        void validate_graph(const CSRGraph &graph) const;
    };

} // namespace hytgraph::graph