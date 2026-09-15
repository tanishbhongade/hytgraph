// src/sep_adapter/detail/engine_runner.hpp
//
// Phase 14 — Data-movement bridge.
//
// PRIVATE header. Not installed, not part of the public API.
// Lives under src/ so that no public header depends on it.
//
// Declares the plain-C++ interface to the CUDA-only engine runner
// implemented in sep_engine_adapter_impl.cu. The bridge
// (hytm_sep_bridge.cpp) is plain C++ and calls run_engine(); the
// .cu translates the request into vendored Engine construction,
// LoadGraph / InitGraph / Start, and result gathering.
//
// CONTRACT: this header includes no vendored header and no CUDA
// header. It is therefore includable from plain C++ TUs.

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "sep_adapter/sep_execution_result.hpp"
#include "sep_adapter/hytm_sep_bridge.hpp"

namespace hytgraph
{
    namespace sep_adapter
    {
        namespace detail
        {

            struct EngineRunRequest
            {
                SEPBridgeAlgorithm algorithm = SEPBridgeAlgorithm::PageRank;

                // Path to the temp file written by sep_host_graph_adapter.
                std::string graph_file_path;

                // Values the .cu will assign to the corresponding gflags.
                // See hytm_sep_bridge.cpp for how these are derived.
                std::string format_flag = "market_big";
                int weight_num_flag = 0;

                // Zero means "leave the gflag at its current value". The .cu
                // restores the previous value after the run finishes.
                std::size_t segment = 0;
                int n_stream = 0;
                int max_iteration = 0;

                // PageRank convergence threshold, passed to the app ctor.
                float pagerank_error = 0.01f;

                // SSSP source vertex. Clamped inside the .cu to the graph's
                // vertex range, matching the original hybrid_sssp.cu.
                std::uint32_t sssp_source = 0;

                // If true, log engine summary to stdout after Start()
                // (mirrors Engine::PrintInfo()).
                bool verbose = false;
            };

            struct EngineRunOutput
            {
                SEPExecutionResult execution;
                SEPBridgeMetrics metrics;

                // Only one of these is populated, selected by `algorithm`.
                // The other is empty.
                std::vector<float> pagerank_values;
                std::vector<std::uint32_t> sssp_distances;
            };

            // Defined in sep_engine_adapter_impl.cu.
            //
            // May throw std::exception on allocation failure. May terminate the
            // process if the vendored engine calls exit() (which Context does
            // when the file cannot be read; there is no way to catch that).
            // All non-fatal error paths return an EngineRunOutput whose
            // `execution.state` is not OK.
            EngineRunOutput run_engine(const EngineRunRequest &request);

        } // namespace detail
    } // namespace sep_adapter
} // namespace hytgraph