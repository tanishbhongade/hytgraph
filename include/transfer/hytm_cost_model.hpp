#pragma once

#include "graph/activity_tracker.hpp"
#include "graph/csr_graph.hpp"
#include "graph/partition.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace hytgraph::transfer
{

    // The three transfer engines considered by HyTGraph's HyTM selector.
    enum class TransferEngine
    {
        ExpTMFilter,
        ExpTMCompaction,
        ImpTMZeroCopy
    };

    // Configuration for the CPU/reference HyTM cost model.
    //
    // Paper-aligned defaults:
    //   alpha = 0.80
    //   beta  = 0.40
    //   gamma = 0.625
    //
    // The paper uses:
    //   m  = 128 bytes
    //   MR = 256 outstanding requests per TLP
    //
    // RTT and CPU compaction throughput are model parameters rather than
    // measured hardware constants in this reproduction.
    struct HyTMCostModelOptions
    {
        // Strict selector threshold for ExpTM-Compaction.
        double alpha = 0.80;

        // Strict selector threshold for comparing ExpTM-Compaction with
        // ImpTM-Zero-Copy.
        double beta = 0.40;

        // Zero-copy RTT mixing factor from the paper.
        double gamma = 0.625;

        // Maximum payload carried by one memory request.
        // Paper-aligned primary value: 128 bytes.
        std::size_t request_payload_bytes = 128U;

        // Maximum outstanding memory requests represented by one TLP.
        // Paper value: MR = 256.
        std::size_t max_requests_per_tlp = 256U;

        // Modeled saturated PCIe round-trip time.
        //
        // The paper notes that RTT may be arbitrarily specified because it
        // can be eliminated in subsequent comparisons. Keeping it explicit
        // makes the reference cost model easier to inspect and test.
        double rtt = 1.0;

        // CPU compaction throughput in bytes/second.
        //
        // This is configurable because the project does not have a
        // reproducible paper-specific measured value.
        double cpu_compaction_throughput_bytes_per_second = 1.0;

        // Size of a destination/neighbor entry in bytes.
        //
        // The current CSR representation stores destination vertex IDs as
        // uint32_t, so the default follows that representation.
        std::size_t destination_entry_bytes =
            sizeof(graph::CSRGraph::vertex_id);

        // Size of one compacted active-vertex index entry in bytes.
        //
        // The compacted representation uses CSRGraph::offset_type for
        // neighbor_index, so the default follows that representation.
        std::size_t vertex_index_bytes =
            sizeof(graph::CSRGraph::offset_type);

        // Alignment boundary used when interpreting the logical CSR
        // neighbor offset for zero-copy alignment overhead.
        //
        // This mirrors the Phase 7 reference/model abstraction rather than
        // claiming to represent a physical host-memory address.
        std::size_t alignment_bytes = 128U;
    };

    // Per-partition activity and derived cost inputs.
    struct HyTMPartitionMetrics
    {
        std::size_t partition_index = 0U;

        graph::CSRGraph::vertex_id vertex_begin = 0U;
        graph::CSRGraph::vertex_id vertex_end = 0U;

        graph::CSRGraph::offset_type active_vertices = 0U;
        graph::CSRGraph::offset_type active_edges = 0U;
        graph::CSRGraph::offset_type total_edges = 0U;

        // Reference byte quantity for ExpTM-Filter:
        // the complete logical partition edge-data payload.
        std::uint64_t filter_transfer_bytes = 0U;

        // Reference byte quantity for ExpTM-Compaction:
        //
        //   active_edge_bytes + active_vertex_index_bytes
        //
        // where the active vertex index is represented by one entry per
        // active source vertex.
        std::uint64_t compaction_bytes = 0U;

        // Sum over active vertices of:
        //
        //   ceil(Do(v) * d1 / m)
        //
        // before adding alignment overhead.
        std::uint64_t zero_copy_memory_requests = 0U;

        // Sum over active vertices of am(v).
        std::uint64_t zero_copy_alignment_overhead = 0U;

        // Total zero-copy requests used by Formula (3):
        //
        //   zero_copy_memory_requests +
        //   zero_copy_alignment_overhead
        std::uint64_t zero_copy_total_requests = 0U;
    };

    // Cost values for one logical partition.
    //
    // These are analytical/model values, not measured wall-clock timings.
    struct HyTMPartitionCosts
    {
        HyTMPartitionMetrics metrics;

        // ExpTM-Filter analytical cost.
        double filter_cost = 0.0;

        // ExpTM-Compaction analytical cost.
        //
        // Includes modeled transfer time plus modeled CPU compaction time.
        double compaction_cost = 0.0;

        // ImpTM-Zero-Copy analytical cost.
        double zero_copy_cost = 0.0;

        // Partition-specific zero-copy round-trip time.
        double zero_copy_rtt = 0.0;

        // Selected transfer engine using the strict comparison rules from
        // the paper.
        TransferEngine selected_engine =
            TransferEngine::ExpTMFilter;
    };

    // Phase 8 CPU/reference HyTM cost analyzer.
    //
    // Responsibilities are intentionally limited to:
    //   - deriving per-partition cost inputs;
    //   - evaluating the three paper cost equations;
    //   - computing RTT_zc;
    //   - applying the deterministic engine-selection rule.
    //
    // It does not implement task combining, CUDA scheduling, Subway
    // execution, GPU-side selection, or multi-stream execution.
    class HyTMCostModel
    {
    public:
        explicit HyTMCostModel(
            HyTMCostModelOptions options = {});

        [[nodiscard]] const HyTMCostModelOptions &
        options() const noexcept;

        // Compute analytical costs and the selected engine for one logical
        // partition.
        //
        // The supplied partition must be a valid CSR-aligned logical
        // partition for the graph, and the activity tracker must correspond
        // to graph.num_vertices().
        [[nodiscard]] HyTMPartitionCosts evaluate_partition(
            const graph::CSRGraph &graph,
            const graph::LogicalPartition &partition,
            const graph::ActivityTracker &activity,
            std::size_t partition_index = 0U) const;

        // Evaluate all logical partitions independently.
        //
        // The ordering of the returned vector matches the ordering of the
        // supplied partitions.
        [[nodiscard]] std::vector<HyTMPartitionCosts>
        evaluate_partitions(
            const graph::CSRGraph &graph,
            const std::vector<graph::LogicalPartition> &partitions,
            const graph::ActivityTracker &activity) const;

        // Apply only the paper's deterministic selection logic to already
        // computed costs. This helper makes the strict boundary behavior
        // directly testable.
        [[nodiscard]] static TransferEngine select_engine(
            double filter_cost,
            double compaction_cost,
            double zero_copy_cost,
            double alpha,
            double beta);

    private:
        HyTMCostModelOptions options_;
    };

} // namespace hytgraph::transfer