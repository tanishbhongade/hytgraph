#pragma once

#include "graph/activity_tracker.hpp"
#include "graph/csr_graph.hpp"
#include "graph/partition.hpp"

#include <cstddef>
#include <vector>

namespace hytgraph::transfer
{

    // Host-side compacted representation of one logical partition.
    //
    // Only active source vertices are represented. Their neighbors are
    // written contiguously in source-vertex order.
    //
    // neighbor_index[i] and neighbor_index[i + 1] delimit the compacted
    // neighbor range belonging to active_vertices[i].
    //
    // Therefore:
    //
    //   active_vertices.size() + 1 == neighbor_index.size()
    //
    // when the representation is valid.
    struct CompactedPartition
    {
        std::size_t partition_index = 0;

        graph::CSRGraph::vertex_id vertex_begin = 0;
        graph::CSRGraph::vertex_id vertex_end = 0;

        // Active source vertices in ascending vertex-ID order.
        std::vector<graph::CSRGraph::vertex_id> active_vertices;

        // Destination IDs for the active vertices, packed contiguously.
        std::vector<graph::CSRGraph::vertex_id> neighbors;

        // Compressed neighbor index.
        //
        // For active_vertices[i], its compacted neighbors occupy:
        //
        //   [neighbor_index[i], neighbor_index[i + 1])
        //
        // The final entry is therefore the total number of compacted
        // neighbors.
        std::vector<graph::CSRGraph::offset_type> neighbor_index;

        [[nodiscard]] graph::CSRGraph::offset_type
        active_edge_count() const noexcept;

        [[nodiscard]] std::size_t
        active_vertex_count() const noexcept;

        [[nodiscard]] std::size_t
        neighbor_bytes() const noexcept;

        [[nodiscard]] std::size_t
        index_bytes() const noexcept;

        [[nodiscard]] std::size_t
        total_bytes() const noexcept;
    };

    // Result of the reference CPU ExpTM-Compaction operation.
    //
    // This represents the compacted transfer payload. It does not execute
    // cudaMemcpy or a GPU computation kernel.
    struct CompactionResult
    {
        std::vector<CompactedPartition> partitions;

        // Wall-clock CPU time spent by the reference compaction operation.
        // This is a measured CPU-side value when the implementation is run.
        double compaction_seconds = 0.0;

        [[nodiscard]] std::size_t
        compacted_partition_count() const noexcept;

        [[nodiscard]] graph::CSRGraph::offset_type
        active_edge_count() const noexcept;

        [[nodiscard]] graph::CSRGraph::offset_type
        active_vertex_count() const noexcept;

        [[nodiscard]] std::size_t
        neighbor_bytes() const noexcept;

        [[nodiscard]] std::size_t
        index_bytes() const noexcept;

        [[nodiscard]] std::size_t
        total_bytes() const noexcept;
    };

    // Reference CPU implementation of HyTGraph's ExpTM-Compaction engine.
    //
    // For every logical partition containing active vertices, the engine:
    //
    //   1. identifies active source vertices;
    //   2. copies only their outgoing neighbors;
    //   3. writes those neighbors contiguously;
    //   4. regenerates a compressed neighbor index.
    //
    // This is deliberately synchronous and CPU-side. CUDA transfer,
    // SEP-Graph, neighbor shifting, task combining, HyTM selection, and
    // multi-stream execution belong to later phases.
    class ExpTMCompaction
    {
    public:
        explicit ExpTMCompaction(bool enabled = true) noexcept;

        [[nodiscard]] bool enabled() const noexcept;

        // Compact active data in every logical partition containing at
        // least one active vertex.
        //
        // If disabled, returns an empty result without performing
        // compaction. This permits an independent Phase 6 ablation.
        [[nodiscard]] CompactionResult compact(
            const graph::CSRGraph &graph,
            const std::vector<graph::LogicalPartition> &partitions,
            const graph::ActivityTracker &activity) const;

        // Expand a compacted partition back into its active source/neighbor
        // representation. This is a correctness/reference helper and does
        // not access the original CSR graph.
        [[nodiscard]] static std::vector<graph::CSRGraph::vertex_id>
        expanded_neighbors(const CompactedPartition &partition);

    private:
        bool enabled_;
    };

} // namespace hytgraph::transfer