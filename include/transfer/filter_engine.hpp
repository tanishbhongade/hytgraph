#pragma once

#include "graph/activity_tracker.hpp"
#include "graph/csr_graph.hpp"
#include "graph/partition.hpp"

#include <cstddef>
#include <vector>

namespace hytgraph::transfer
{

    struct FilterPartitionDecision
    {
        std::size_t partition_index = 0;

        bool active = false;

        // True when this partition is selected for transfer.
        bool transferred = false;

        graph::CSRGraph::offset_type active_edges = 0;
        graph::CSRGraph::offset_type total_edges = 0;

        // ExpTM-Filter transfers the complete partition when selected.
        graph::CSRGraph::offset_type transferred_edges = 0;
        std::size_t transferred_bytes = 0;
    };

    struct FilterTransferPlan
    {
        std::vector<FilterPartitionDecision> partitions;

        [[nodiscard]] std::size_t active_partition_count() const noexcept;
        [[nodiscard]] std::size_t transferred_partition_count() const noexcept;

        [[nodiscard]] graph::CSRGraph::offset_type
        transferred_edge_count() const noexcept;

        [[nodiscard]] std::size_t
        transferred_byte_count() const noexcept;
    };

    // Reference CPU implementation of HyTGraph's ExpTM-Filter mechanism.
    //
    // A logical partition containing at least one active edge is transferred
    // in full. A partition containing no active edges is skipped.
    //
    // This class builds a transfer plan; it does not perform CUDA memory
    // copies. Actual GPU transfer execution belongs to later runtime
    // integration.
    class ExpTMFilter
    {
    public:
        explicit ExpTMFilter(bool enabled = true) noexcept;

        [[nodiscard]] bool enabled() const noexcept;

        [[nodiscard]] FilterTransferPlan plan(
            const graph::CSRGraph &graph,
            const std::vector<graph::LogicalPartition> &partitions,
            const graph::ActivityTracker &activity) const;

    private:
        bool enabled_;
    };

} // namespace hytgraph::transfer