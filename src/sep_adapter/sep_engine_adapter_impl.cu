// src/sep_adapter/sep_engine_adapter_impl.cu
//
// Phase 14 — Data-movement bridge.
//
// CUDA-only translation unit that constructs and drives the vendored
// sepgraph::engine::Engine. This is the ONLY file in Phase 14 that
// includes <framework/framework.cuh>.
//
// Compilation: LANGUAGE CUDA. See CMakeLists.txt additions in File 8.
//
// Responsibilities:
//   * Define every gflags the vendored headers DECLARE but do not
//     DEFINE (the original samples define these in their main.cpp;
//     we do not link those).
//   * Save/restore gflags around the run so the bridge does not leak
//     global state.
//   * Instantiate Engine<...> for PageRank and SSSP.
//   * Call LoadGraph / InitGraph / Start.
//   * Gather final values (rank scores / distances).
//
// Limitations documented at the handoff:
//   * Engine does not expose RunningInfo through its public API, so
//     per-engine partition counters (explicit_num / zerocopy_num /
//     compaction_num) and time counters are left at 0 in Phase 14.
//     Phase 15 will revisit via JsonWriter or a project-owned wrapper.
//   * Engine calls exit() from inside Context if the graph file
//     cannot be read. This file cannot catch that; the bridge
//     ensures the file is written and readable before calling.

// ---------------------------------------------------------------------------
// gflags definitions
// ---------------------------------------------------------------------------
//
// The vendored headers DECLARE these but do not DEFINE them. Defaults
// match the vendored build where known; otherwise they are chosen to
// match behaviour observed in the original HyTGraph samples.

#include <gflags/gflags.h>

// Flag definitions live in sep_flags.cpp. They must not appear in
// this TU because the vendored headers below also DECLARE some of
// these flags; a DEFINE + DECLARE in the same TU is a compile error.

// ---------------------------------------------------------------------------
// Includes
// ---------------------------------------------------------------------------

#include "sep_adapter/detail/engine_runner.hpp"
#include "sep_adapter/apps/pagerank_app.hpp"
#include "sep_adapter/apps/sssp_app.hpp"

#include <framework/framework.cuh>

#include <cstdint>
#include <exception>
#include <string>
#include <vector>
#include <type_traits>

namespace hytgraph
{
    namespace sep_adapter
    {
        namespace detail
        {

            namespace
            {

                // ---------------------------------------------------------------------------
                // ensure_graph_datum_bitmaps
                // ---------------------------------------------------------------------------
                //
                // WORKAROUND for a defect in the vendored snapshot.
                //
                //   sepgraph::graphs::GraphDatum's constructor does not allocate the
                //   four CompressedBitmap members (m_wl_bitmap_in, m_wl_bitmap_out_high,
                //   m_wl_bitmap_out_low, m_wl_bitmap_middle). RunSyncPushDDB reads
                //   m_wl_bitmap_out_high.DeviceObject(), whose assertion (m_size > 0)
                //   fires on the first iteration.
                //
                //   RebuildBitmapWorklist() exists in algo_variants.cuh but is never
                //   called by framework.cuh, so nothing in the vendored lifecycle
                //   ever sizes the bitmaps.
                //
                // We cannot modify vendored sources (PROJECT_STATE.md §20). This
                // helper sizes the four bitmaps from our side using only the public
                // CompressedBitmap API (size_t constructor + Swap).
                //
                // Documented in the Phase 14 handoff as a project-side workaround.

                template <typename GraphDatumType>
                void ensure_graph_datum_bitmaps(GraphDatumType &gd)
                {
                    using BitmapType = std::decay_t<decltype(gd.m_wl_bitmap_in)>;

                    auto fix = [](BitmapType &bm, std::size_t nnodes)
                    {
                        if (bm.GetSize() == 0)
                        {
                            BitmapType tmp(nnodes);
                            bm.Swap(tmp);
                            // tmp destructs; its m_size is 0, so Free() is a no-op.
                        }
                    };

                    const std::size_t n = static_cast<std::size_t>(gd.nnodes);
                    fix(gd.m_wl_bitmap_in, n);
                    fix(gd.m_wl_bitmap_out_high, n);
                    fix(gd.m_wl_bitmap_out_low, n);
                    fix(gd.m_wl_bitmap_middle, n);
                }

                // ---------------------------------------------------------------------------
                // FlagsGuard
                // ---------------------------------------------------------------------------
                //
                // Saves the gflags values the vendored Context / Engine read during
                // construction. Restores on scope exit. Not thread-safe.

                struct FlagsGuard
                {
                    std::string saved_graphfile;
                    std::string saved_format;
                    int32_t saved_weight_num;
                    int32_t saved_segment;
                    int32_t saved_n_stream;
                    int32_t saved_max_iteration;
                    bool saved_gen_graph;
                    bool restored = false;

                    FlagsGuard()
                    {
                        saved_graphfile = FLAGS_graphfile;
                        saved_format = FLAGS_format;
                        saved_weight_num = FLAGS_weight_num;
                        saved_segment = FLAGS_SEGMENT;
                        saved_n_stream = FLAGS_n_stream;
                        saved_max_iteration = FLAGS_max_iteration;
                        saved_gen_graph = FLAGS_gen_graph;
                    }
                    ~FlagsGuard() { restore(); }

                    void restore()
                    {
                        if (restored)
                            return;
                        FLAGS_graphfile = saved_graphfile;
                        FLAGS_format = saved_format;
                        FLAGS_weight_num = saved_weight_num;
                        FLAGS_SEGMENT = saved_segment;
                        FLAGS_n_stream = saved_n_stream;
                        FLAGS_max_iteration = saved_max_iteration;
                        FLAGS_gen_graph = saved_gen_graph;
                        restored = true;
                    }
                };

                // ---------------------------------------------------------------------------
                // PageRank runner
                // ---------------------------------------------------------------------------

                SEPExecutionResult run_pagerank(const EngineRunRequest &req,
                                                EngineRunOutput &out)
                {
                    using TValue = float;
                    using TBuffer = float;
                    using TWeight = groute::graphs::NoWeight;

                    using EngineType = sepgraph::engine::Engine<
                        TValue, TBuffer, TWeight,
                        hytgraph::sep_adapter::apps::PageRank,
                        double>;

                    EngineType engine(sepgraph::policy::AlgoType::ITERATIVE_SCHEME);

                    sepgraph::engine::EngineOptions engine_opt;
                    engine.SetOptions(engine_opt);

                    engine.LoadGraph();

                    // Work around the vendored snapshot's missing bitmap allocation.
                    {
                        const auto &gd_ref = engine.GetGraphDatum();
                        using GraphDatumType = std::decay_t<decltype(gd_ref)>;
                        auto &gd_mut = const_cast<GraphDatumType &>(gd_ref);
                        ensure_graph_datum_bitmaps(gd_mut);
                    }

                    // InitGraph(UnusedData&...) — PageRank takes a single `double`.
                    double error = static_cast<double>(req.pagerank_error);
                    engine.InitGraph(error);

                    engine.Start(); // PageRank uses default priority_delta = 0.

                    if (req.verbose)
                        engine.PrintInfo();

                    const auto &values = engine.GatherValue();
                    out.pagerank_values.assign(values.begin(), values.end());

                    const auto &gd = engine.GetGraphDatum();
                    out.metrics.nnodes = gd.nnodes;
                    out.metrics.nedges = gd.nedges;
                    // current_round, engine counters, and times live in
                    // Engine::m_running_info, which has no public accessor.
                    // Left at 0; documented at the handoff.

                    return make_ok("PageRank run completed");
                }

                // ---------------------------------------------------------------------------
                // SSSP runner
                // ---------------------------------------------------------------------------

                SEPExecutionResult run_sssp(const EngineRunRequest &req,
                                            EngineRunOutput &out)
                {
                    using TValue = std::uint32_t;
                    using TBuffer = std::uint32_t;
                    using TWeight = std::uint32_t;

                    using EngineType = sepgraph::engine::Engine<
                        TValue, TBuffer, TWeight,
                        hytgraph::sep_adapter::apps::SSSP,
                        index_t>;

                    EngineType engine(sepgraph::policy::AlgoType::TRAVERSAL_SCHEME);

                    sepgraph::engine::EngineOptions engine_opt;
                    engine.SetOptions(engine_opt);

                    engine.LoadGraph();

                    // Work around the vendored snapshot's missing bitmap allocation.
                    {
                        const auto &gd_ref = engine.GetGraphDatum();
                        using GraphDatumType = std::decay_t<decltype(gd_ref)>;
                        auto &gd_mut = const_cast<GraphDatumType &>(gd_ref);
                        ensure_graph_datum_bitmaps(gd_mut);
                    }

                    // Clamp source to the graph's vertex range (matches the original
                    // hybrid_sssp.cu behaviour).
                    const auto &gd = engine.GetGraphDatum();
                    index_t source = static_cast<index_t>(req.sssp_source);
                    if (gd.nnodes > 0 && source >= gd.nnodes)
                    {
                        source = gd.nnodes - 1;
                    }

                    // InitGraph(UnusedData&...) — SSSP takes a single `index_t`.
                    engine.InitGraph(source);

                    // Priority delta heuristic from the original hybrid_sssp.cu:
                    //   Δ = 32 · avg_weight / avg_degree
                    // Only meaningful when the graph has edges and weights.
                    const auto &host_csr = engine.CSRGraph();
                    int init_prio = 0;
                    if (host_csr.nedges > 0 && host_csr.nnodes > 0 && host_csr.edge_weights != nullptr)
                    {
                        double weight_sum = 0.0;
                        for (std::uint64_t e = 0; e < host_csr.nedges; ++e)
                        {
                            weight_sum += static_cast<double>(host_csr.edge_weights[e]);
                        }
                        const double avg_weight = weight_sum / host_csr.nedges;
                        const double avg_degree =
                            static_cast<double>(host_csr.nedges) / host_csr.nnodes;
                        init_prio = static_cast<int>(32.0 * avg_weight / avg_degree);
                    }

                    engine.Start(init_prio);

                    if (req.verbose)
                        engine.PrintInfo();

                    const auto &values = engine.GatherValue();
                    out.sssp_distances.assign(values.begin(), values.end());

                    out.metrics.nnodes = gd.nnodes;
                    out.metrics.nedges = gd.nedges;

                    return make_ok("SSSP run completed");
                }

            } // namespace

            // ---------------------------------------------------------------------------
            // run_engine
            // ---------------------------------------------------------------------------

            EngineRunOutput run_engine(const EngineRunRequest &request)
            {
                EngineRunOutput out;

                FlagsGuard guard;

                FLAGS_graphfile = request.graph_file_path;
                FLAGS_format = request.format_flag;
                FLAGS_weight_num = request.weight_num_flag;
                FLAGS_gen_graph = false;

                // Push load balancing.
                //
                // The vendored default is FINE_GRAINED. In that mode,
                // CTAWorkSchedulerNew asserts (np_local.size > 0) on every thread
                // in a block that has no work item. With a block size of 256 and
                // a work source smaller than 256, threads 100..255 have no work
                // and trip the assertion. This is a vendored invariant that does
                // not hold for our out-of-core setup, where per-segment active
                // sets are naturally smaller than a block.
                //
                // RelaxCTADB (sync_push_dd.cuh:222-242) handles three LB modes:
                // COARSE_GRAINED, FINE_GRAINED, HYBRID. LoadBalancing::NONE falls
                // through to `default: assert(false);` and is not implemented.
                // COARSE_GRAINED assigns one block per work item and does not
                // depend on having at least blockDim threads per item.
                //
                // Recorded in the Phase 14 handoff as a project-side workaround.
                // FLAGS_lb_push = "hybrid";

                if (request.segment > 0)
                {
                    FLAGS_SEGMENT = static_cast<int32_t>(request.segment);
                }
                if (request.n_stream > 0)
                {
                    FLAGS_n_stream = request.n_stream;
                }
                if (request.max_iteration > 0)
                {
                    FLAGS_max_iteration = request.max_iteration;
                }

                switch (request.algorithm)
                {
                case SEPBridgeAlgorithm::PageRank:
                    out.execution = run_pagerank(request, out);
                    break;
                case SEPBridgeAlgorithm::SSSP:
                    out.execution = run_sssp(request, out);
                    break;
                }

                return out;
            }

        } // namespace detail
    } // namespace sep_adapter
} // namespace hytgraph