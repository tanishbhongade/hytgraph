// include/sep_adapter/hytm_sep_bridge.hpp
//
// Phase 14 — Data-movement bridge.
//
// Public entry point for driving one vendored SEP-Graph engine run
// from a project-owned CSRGraph.
//
// Design summary (Option A, confirmed against the vendored snapshot):
//   1. Serialize the project CSR to a temp file in the
//      "market_big" layout.
//   2. Save gflags values the vendored Context/Engine read in their
//      constructors; set ours.
//   3. Construct the vendored Engine, LoadGraph, InitGraph, Start.
//   4. Gather results.
//   5. Restore flags, delete temp file.
//
// The vendored Engine has no per-partition GraphDatum injection API.
// It runs its own segment loop internally, dispatches to the three
// HyTM engines (Exp_Filter, Exp_Compaction, Zero_Copy) via
// PolicyDecisionMaker, and stops on convergence. This bridge
// therefore exposes one "run to convergence" call, not a per-step
// execute_partition API. This is a documented deviation from
// MASTER_PLAN §Phase 14; see the Phase 14 handoff for details.
//
// PUBLIC BOUNDARY: this header includes no vendored header. All
// vendored interaction happens inside the .cu translation unit that
// defines Impl.
//
// THREAD SAFETY: the vendored engine reads gflags globals in its
// constructor. Only one bridge may be constructed and run at a time
// within a process. Concurrent construction is undefined.

#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "graph/csr_graph.hpp"
#include "sep_adapter/sep_execution_result.hpp"
#include "sep_adapter/sep_host_graph_adapter.hpp"

namespace hytgraph
{
    namespace sep_adapter
    {

        using hytgraph::graph::CSRGraph;

        // ---------------------------------------------------------------------------
        // Algorithm selection
        // ---------------------------------------------------------------------------

        enum class SEPBridgeAlgorithm : std::uint8_t
        {
            PageRank = 0, // Engine<float, float, NoWeight, PageRank, double>
            SSSP = 1,     // Engine<uint32_t, uint32_t, uint32_t, SSSP, uint32_t>
        };

        constexpr std::uint8_t kSEPBridgeAlgorithmCount = 2;

        const char *to_string(SEPBridgeAlgorithm algo) noexcept;
        std::optional<SEPBridgeAlgorithm> from_string(const std::string &s) noexcept;

        // ---------------------------------------------------------------------------
        // Configuration
        // ---------------------------------------------------------------------------

        struct SEPBridgeConfig
        {
            // Vendored graph_t layout. `weighted` is FORCED by `algorithm`
            // inside the bridge constructor: PageRank writes an unweighted
            // file; SSSP writes a weighted file. Any user-provided value
            // is overridden so that the temp file and the engine template
            // always agree.
            SEPGraphFileConfig file_config{};

            // Segment count passed to the vendored engine via FLAGS_SEGMENT.
            // If 0 (default), the bridge auto-computes a value that is valid
            // for the graph size: max(1, min(32, num_vertices / 64)). This
            // is required because the vendored Engine does not clamp
            // FLAGS_SEGMENT to the actual number of non-empty segments, and
            // an oversized value causes an out-of-memory error inside
            // GraphDatum's constructor.
            // If > 0, the caller's value is used as-is.
            std::size_t segment = 0;

            // If > 0, overrides FLAGS_n_stream before engine construction.
            // If 0, leaves the gflag's existing value.
            int n_stream = 0;

            // If > 0, overrides FLAGS_max_iteration. If 0, leaves default
            // (the vendored default is 1000).
            int max_iteration = 0;

            // PageRank convergence threshold. Passed to the PageRank app
            // constructor (UnusedData pack).
            float pagerank_error = 0.01f;

            // SSSP source vertex. Clamped to [0, num_vertices - 1] before
            // use, matching the original hybrid_sssp.cu behaviour.
            std::uint32_t sssp_source = 0;

            // If true, log engine summary to stdout after Start().
            // Mirrors Engine::PrintInfo().
            bool verbose = false;
        };

        // ---------------------------------------------------------------------------
        // Metrics
        // ---------------------------------------------------------------------------
        //
        // Populated from the vendored TRunningInfo after Start() returns.
        // All values are read from the engine; none are modeled.

        struct SEPBridgeMetrics
        {
            std::uint32_t nnodes = 0;
            std::uint64_t nedges = 0;
            std::uint32_t current_round = 0;

            // Per-engine partition counters written by ExecutePolicyBW().
            // Names mirror TRunningInfo fields: explicit_num = Exp_Filter,
            // zerocopy_num = Zero_Copy, compaction_num = Exp_Compaction.
            std::uint32_t explicit_num = 0;
            std::uint32_t zerocopy_num = 0;
            std::uint32_t compaction_num = 0;

            float time_total_ms = 0.0f;
            float time_kernel_ms = 0.0f;
            float time_load_ms = 0.0f;
            float time_init_ms = 0.0f;
        };

        // ---------------------------------------------------------------------------
        // Result
        // ---------------------------------------------------------------------------

        struct SEPBridgeResult
        {
            SEPExecutionResult execution;

            SEPBridgeMetrics metrics;

            // Only populated for PageRank runs. Empty otherwise.
            // Values are the per-vertex final PageRank scores.
            std::vector<float> pagerank_values;

            // Only populated for SSSP runs. Empty otherwise.
            // Values are the per-vertex final distances. UINT32_MAX means
            // unreachable.
            std::vector<std::uint32_t> sssp_distances;
        };

        // ---------------------------------------------------------------------------
        // Bridge
        // ---------------------------------------------------------------------------

        class HyTMSEPBridge
        {
        public:
            // Constructs the bridge and serializes the graph to a temp file.
            // Does NOT construct the vendored engine; that happens inside
            // run(), so that any exception / abort during engine construction
            // does not leave a half-built object.
            //
            // Returns via std::nullopt-like semantics: the constructor does
            // not throw on failure. Call valid() before run(). This mirrors
            // the SEPGraphFile factory pattern.
            HyTMSEPBridge(SEPBridgeAlgorithm algorithm,
                          const CSRGraph &graph,
                          SEPBridgeConfig config = {});

            ~HyTMSEPBridge();

            HyTMSEPBridge(const HyTMSEPBridge &) = delete;
            HyTMSEPBridge &operator=(const HyTMSEPBridge &) = delete;
            HyTMSEPBridge(HyTMSEPBridge &&) noexcept;
            HyTMSEPBridge &operator=(HyTMSEPBridge &&) noexcept;

            // True if construction succeeded and the temp file is on disk.
            bool valid() const noexcept;

            // Runs the engine to convergence. Idempotent: subsequent calls
            // after the first return FAILED. The engine's internal state is
            // consumed by the first Start() call.
            SEPBridgeResult run();

            // Path to the temp file. Exposed for diagnostics only. Empty
            // if !valid().
            const std::string &graph_file_path() const noexcept;

            // Algorithm this bridge was constructed with.
            SEPBridgeAlgorithm algorithm() const noexcept;

        private:
            struct Impl;
            std::unique_ptr<Impl> impl_;
        };

    } // namespace sep_adapter
} // namespace hytgraph