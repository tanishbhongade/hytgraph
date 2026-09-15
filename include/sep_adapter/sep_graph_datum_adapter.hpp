// include/sep_adapter/sep_graph_datum_adapter.hpp
//
// Phase 14 — Data-movement bridge.
//
// Project-owned builder for the vendored sepgraph::graphs::GraphDatum.
//
// Design constraints:
//   * pimpl: no vendored symbol appears in this header.
//   * This header is includable in both ON and OFF builds. In OFF
//     builds, the builder returns nullptr; callers must handle that.
//   * Worklist seeding (initial active vertex queue) is NOT performed
//     here. That belongs to the bridge (hytm_sep_bridge) because
//     worklist source depends on the algorithm, not on the payload
//     shape.

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "graph/csr_graph.hpp" // Phase 1 project-owned CSR
#include "sep_adapter/sep_algorithm_mapper.hpp"

namespace hytgraph
{
    namespace sep_adapter
    {

        // ---------------------------------------------------------------------------
        // Construction mode
        // ---------------------------------------------------------------------------
        //
        // Mirrors the three HyTM transfer engines at the GraphDatum level.
        // This enum is deliberately separate from the bridge-level
        // TransferEngineType so that this header does not depend on the
        // bridge header, and so that Phase 15 can introduce new modes without
        // touching the bridge.

        enum class GraphDatumMode : std::uint8_t
        {
            Filter = 0,     // whole partition's edges reach the GPU
            Compaction = 1, // compacted active subgraph, new index array
            ZeroCopy = 2,   // pinned-memory active subgraph, no explicit copy
        };

        constexpr std::uint8_t kGraphDatumModeCount = 3;

        const char *to_string(GraphDatumMode mode) noexcept;

        // ---------------------------------------------------------------------------
        // Build configuration
        // ---------------------------------------------------------------------------

        struct GraphDatumBuildConfig
        {
            // Number of vendored GraphDatum segments. The bridge always uses
            // segment == 1 in Phase 14 (one logical partition per GraphDatum).
            // Multi-segment construction is reserved for Phase 15 task
            // combining.
            std::size_t segment = 1;

            // If true, GraphDatum is allocated with pinned host memory.
            // Required for ZeroCopy mode. Ignored for Filter mode.
            // Compaction mode may or may not use pinned memory depending on
            // whether the compacted buffer is intended for zero-copy access.
            bool on_pinned_memory = false;

            // Upper bound on edge count per segment. If 0, the builder
            // computes it as `graph.num_edges` (conservative, correct, slower
            // allocation).
            std::uint64_t seg_max_edge = 0;
        };

        // ---------------------------------------------------------------------------
        // Opaque payload
        // ---------------------------------------------------------------------------
        //
        // Consumers must never inspect the payload directly. The only legal
        // operations are:
        //   * passing it to SEPExecutionDriver::load_graph_datum (Phase 14
        //     interface extension, see PROJECT_STATE.md);
        //   * the diagnostic accessors declared below;
        //   * destroying it.

        struct GraphDatumPayload;

        // ---------------------------------------------------------------------------
        // Builder
        // ---------------------------------------------------------------------------
        //
        // active_vertices is assumed sorted and deduplicated. The builder does
        // NOT re-sort, because the bridge performs the sort once per step and
        // reuses it across all partitions.
        //
        // Returns nullptr if:
        //   * HYTGRAPH_WITH_SEP_GRAPH == 0;
        //   * active_vertices is empty (nothing to do);
        //   * any u32 in active_vertices is >= graph.num_vertices;
        //   * an internal allocation fails.
        //
        // The returned payload owns all vendored allocations and releases them
        // on destruction. The payload must be moved into the driver, never
        // copied.

        std::unique_ptr<GraphDatumPayload>
        build_graph_datum_payload(const CSRGraph &graph,
                                  const std::vector<std::uint32_t> &active_vertices,
                                  GraphDatumMode mode,
                                  const GraphDatumBuildConfig &config = {});

        // ---------------------------------------------------------------------------
        // Diagnostics
        // ---------------------------------------------------------------------------
        //
        // These are the only non-construction operations permitted on a
        // payload. They are used by tests and by the bridge's metrics layer.

        std::size_t payload_num_vertices(const GraphDatumPayload &p) noexcept;
        std::size_t payload_num_edges(const GraphDatumPayload &p) noexcept;
        std::size_t payload_num_segments(const GraphDatumPayload &p) noexcept;

        // True if the payload was allocated with pinned host memory.
        bool payload_is_pinned(const GraphDatumPayload &p) noexcept;

    } // namespace sep_adapter
} // namespace hytgraph