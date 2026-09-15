// src/sep_adapter/hytm_sep_bridge.cpp
//
// Phase 14 — Data-movement bridge.
//
// Plain C++ implementation. Delegates the CUDA-only work to
// detail::run_engine(), defined in sep_engine_adapter_impl.cu.
//
// This TU must never include a vendored header or a CUDA header.

#include "sep_adapter/hytm_sep_bridge.hpp"

#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <algorithm>

#include "sep_adapter/detail/engine_runner.hpp"

namespace hytgraph
{
    namespace sep_adapter
    {
        using hytgraph::graph::CSRGraph;
        // ---------------------------------------------------------------------------
        // Algorithm helpers
        // ---------------------------------------------------------------------------

        const char *to_string(SEPBridgeAlgorithm algo) noexcept
        {
            switch (algo)
            {
            case SEPBridgeAlgorithm::PageRank:
                return "PageRank";
            case SEPBridgeAlgorithm::SSSP:
                return "SSSP";
            }
            return "Unknown";
        }

        std::optional<SEPBridgeAlgorithm>
        from_string(const std::string &s) noexcept
        {
            if (s == "PageRank")
                return SEPBridgeAlgorithm::PageRank;
            if (s == "SSSP")
                return SEPBridgeAlgorithm::SSSP;
            return std::nullopt;
        }

        namespace
        {

            bool algorithm_requires_weights(SEPBridgeAlgorithm algo) noexcept
            {
                return algo == SEPBridgeAlgorithm::SSSP;
            }

        } // namespace

        // ---------------------------------------------------------------------------
        // Impl
        // ---------------------------------------------------------------------------

        struct HyTMSEPBridge::Impl
        {
            SEPBridgeAlgorithm algorithm;
            SEPBridgeConfig config;
            SEPGraphFile file;
            bool consumed = false;

            Impl(SEPBridgeAlgorithm a, SEPBridgeConfig c)
                : algorithm(a), config(std::move(c)) {}
        };

        // ---------------------------------------------------------------------------
        // Lifecycle
        // ---------------------------------------------------------------------------

        HyTMSEPBridge::HyTMSEPBridge(SEPBridgeAlgorithm algorithm,
                                     const CSRGraph &graph,
                                     SEPBridgeConfig config)
            : impl_(std::make_unique<Impl>(algorithm, std::move(config)))
        {
            // The algorithm dictates whether the vendored graph_t must be
            // weighted. PageRank runs on Engine<..., NoWeight, ...>;
            // SSSP runs on Engine<..., uint32_t, ...>. Honor the algorithm;
            // override any conflicting user-provided setting so that the
            // temp file and the engine template agree.
            impl_->config.file_config.weighted =
                algorithm_requires_weights(impl_->algorithm);

            // Auto-choose the segment count if the caller did not.
            //
            // Vendored bug: Engine::LoadGraph does not clamp FLAGS_SEGMENT to
            // the actual number of non-empty segments. For a small graph, it
            // computes 1 segment but leaves FLAGS_SEGMENT = 32; GraphDatum's
            // constructor then indexes nnodes_num[0..31] past the vector's
            // end, reads garbage, and cudaMalloc's a huge queue. Capping at
            // a value derived from the graph size avoids this.
            //
            // Heuristic: 64 vertices per segment, floor 1, ceiling 32. For
            // large graphs this saturates at the vendored default of 32.
            if (impl_->config.segment == 0)
            {
                const std::size_t n =
                    static_cast<std::size_t>(graph.num_vertices());
                const std::size_t auto_seg =
                    std::max<std::size_t>(1,
                                          std::min<std::size_t>(32, n / 64));
                impl_->config.segment = auto_seg;
            }

            if (auto file = write_sep_graph_file(graph, impl_->config.file_config))
            {
                impl_->file = std::move(*file);
            }
        }

        HyTMSEPBridge::~HyTMSEPBridge() = default;

        HyTMSEPBridge::HyTMSEPBridge(HyTMSEPBridge &&) noexcept = default;
        HyTMSEPBridge &HyTMSEPBridge::operator=(HyTMSEPBridge &&) noexcept = default;

        // ---------------------------------------------------------------------------
        // Observers
        // ---------------------------------------------------------------------------

        bool HyTMSEPBridge::valid() const noexcept
        {
            return impl_ != nullptr && impl_->file.valid();
        }

        const std::string &HyTMSEPBridge::graph_file_path() const noexcept
        {
            static const std::string kEmpty;
            if (!impl_)
                return kEmpty;
            return impl_->file.path();
        }

        SEPBridgeAlgorithm HyTMSEPBridge::algorithm() const noexcept
        {
            return impl_ ? impl_->algorithm : SEPBridgeAlgorithm::PageRank;
        }

        // ---------------------------------------------------------------------------
        // run()
        // ---------------------------------------------------------------------------

        SEPBridgeResult HyTMSEPBridge::run()
        {
            SEPBridgeResult result;

            if (!impl_)
            {
                result.execution = make_failed("HyTMSEPBridge: not constructed");
                return result;
            }
            if (!impl_->file.valid())
            {
                result.execution =
                    make_failed("HyTMSEPBridge: graph temp file is not valid");
                return result;
            }
            if (impl_->consumed)
            {
                result.execution =
                    make_failed("HyTMSEPBridge: run() already consumed this bridge");
                return result;
            }

            detail::EngineRunRequest request;
            request.algorithm = impl_->algorithm;
            request.graph_file_path = impl_->file.path();
            request.format_flag = impl_->file.format_flag_value();
            request.weight_num_flag = impl_->file.weight_num_flag_value();
            request.segment = impl_->config.segment;
            request.n_stream = impl_->config.n_stream;
            request.max_iteration = impl_->config.max_iteration;
            request.pagerank_error = impl_->config.pagerank_error;
            request.sssp_source = impl_->config.sssp_source;
            request.verbose = impl_->config.verbose;

            // Mark consumed before invoking the runner, so that a throwing
            // runner does not leave the bridge in a state where run() can
            // be called again.
            impl_->consumed = true;

            detail::EngineRunOutput out;
            try
            {
                out = detail::run_engine(request);
            }
            catch (const std::exception &e)
            {
                result.execution = make_failed(
                    std::string("HyTMSEPBridge: engine runner threw: ") + e.what());
                return result;
            }
            catch (...)
            {
                result.execution = make_failed(
                    "HyTMSEPBridge: engine runner threw an unknown exception");
                return result;
            }

            result.execution = std::move(out.execution);
            result.metrics = out.metrics;
            result.pagerank_values = std::move(out.pagerank_values);
            result.sssp_distances = std::move(out.sssp_distances);
            return result;
        }

    } // namespace sep_adapter
} // namespace hytgraph