#pragma once

#include "graph/activity_tracker.hpp"
#include "graph/csr_graph.hpp"
#include "graph/partition.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace hytgraph::transfer
{

    enum class ZeroCopyMode
    {
        // No CUDA zero-copy mapping is performed. Metrics are derived from
        // the logical CSR representation and should be treated as modeled.
        Modeled,

        // Host memory was successfully allocated/mapped for device access.
        // This does not imply that a GPU kernel has executed against it.
        MappedHost
    };

    struct ZeroCopyVertexMetrics
    {
        graph::CSRGraph::vertex_id vertex = 0;

        graph::CSRGraph::offset_type degree = 0;

        // Number of memory requests required by this adjacency list under
        // the Phase 7 request model:
        //
        //   ceil(degree * sizeof(vertex_id) / request_payload_bytes)
        //   + alignment_overhead
        //
        // The implementation records the result explicitly so the modeled
        // behavior can be inspected independently of GPU execution.
        std::uint64_t memory_requests = 0;

        // 1 when the adjacency-list byte range is not aligned to the
        // configured request payload boundary, otherwise 0.
        std::uint64_t alignment_overhead = 0;
    };

    struct ZeroCopyPartition
    {
        std::size_t partition_index = 0;

        graph::CSRGraph::vertex_id vertex_begin = 0;
        graph::CSRGraph::vertex_id vertex_end = 0;

        // Active source vertices in ascending vertex-ID order.
        std::vector<graph::CSRGraph::vertex_id> active_vertices;

        // Request/alignment metrics for each active source vertex.
        //
        // The i-th entry corresponds to active_vertices[i].
        std::vector<ZeroCopyVertexMetrics> vertex_metrics;

        graph::CSRGraph::offset_type active_edge_count = 0;
        std::uint64_t memory_request_count = 0;
        std::uint64_t alignment_overhead_count = 0;
        std::uint64_t modeled_tlp_count = 0;
    };

    struct ZeroCopyResult
    {
        ZeroCopyMode mode = ZeroCopyMode::Modeled;

        std::vector<ZeroCopyPartition> partitions;

        // Number of partitions containing at least one active vertex.
        std::size_t active_partition_count = 0;

        graph::CSRGraph::offset_type active_vertex_count = 0;
        graph::CSRGraph::offset_type active_edge_count = 0;

        std::uint64_t memory_request_count = 0;
        std::uint64_t alignment_overhead_count = 0;
        std::uint64_t modeled_tlp_count = 0;

        // True only when an actual CUDA host-mapping operation succeeded.
        //
        // This is intentionally separate from mode so callers can distinguish
        // "mapped host memory exists" from "GPU execution actually happened."
        bool host_memory_mapped = false;

        // CPU-side time spent building the Phase 7 representation/metrics.
        // This is a measured host-side value when executed.
        double preparation_seconds = 0.0;
    };

    struct ZeroCopyOptions
    {
        // Maximum bytes carried by one modeled memory request.
        //
        // The paper discusses 32, 64, 96, and 128-byte request payloads and
        // uses 128 bytes for its primary modeling. 128 is therefore the
        // project default.
        std::size_t request_payload_bytes = 128U;

        // Maximum outstanding memory requests represented by one modeled
        // TLP. The paper uses MR = 256.
        std::size_t max_requests_per_tlp = 256U;

        // When true and CUDA support is available, attempt to allocate host
        // memory suitable for GPU mapping. When false, the implementation
        // remains entirely modeled/reference based.
        bool enable_host_mapping = true;

        // Alignment boundary used by the reference/model path.
        //
        // A request is considered aligned when its starting byte offset is
        // divisible by this value.
        std::size_t alignment_bytes = 128U;
    };

    // Phase 7 reference implementation of ImpTM-zero-copy.
    //
    // The engine computes the active vertices and request/alignment metrics
    // needed to model zero-copy access. Where CUDA support is available, it
    // can also allocate page-locked host memory and expose it for device
    // mapping. This phase does not launch a computation kernel.
    class ImpTMZeroCopy
    {
    public:
        explicit ImpTMZeroCopy(
            ZeroCopyOptions options = {}) noexcept;

        [[nodiscard]] const ZeroCopyOptions &
        options() const noexcept;

        // Analyze the active graph data partition-by-partition and produce
        // the zero-copy reference/model representation.
        //
        // The method validates that:
        //   - activity matches the graph vertex count;
        //   - logical partitions are valid CSR-aligned ranges;
        //   - request configuration is valid.
        //
        // No graph data is modified.
        [[nodiscard]] ZeroCopyResult prepare(
            const graph::CSRGraph &graph,
            const std::vector<graph::LogicalPartition> &partitions,
            const graph::ActivityTracker &activity) const;

    private:
        ZeroCopyOptions options_;
    };

} // namespace hytgraph::transfer