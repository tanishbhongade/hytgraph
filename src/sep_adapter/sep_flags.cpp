// src/sep_adapter/sep_flags.cpp
//
// Phase 14 — Data-movement bridge.
//
// Central definition of every gflags value the vendored SEP-Graph
// headers DECLARE or use.
//
// Kept in its own plain-C++ translation unit so that no TU which
// includes a vendored header ever contains a DEFINE_* macro for a
// flag the vendored code also DECLAREs. gflags rejects the duplicate.
//
// This file must never include any vendored header or CUDA header.

#include <gflags/gflags.h>

// Graph source.
DEFINE_string(graphfile, "", "Path to graph file");
DEFINE_string(format, "market_big", "Graph file format");
DEFINE_int32(weight_num, 0, "Weight mode for market_big (0=int, 1=synthetic)");
DEFINE_bool(gen_graph, false, "Generate synthetic graph instead of reading file");
DEFINE_int32(gen_nnodes, 100, "Synthetic graph vertex count");
DEFINE_int32(gen_factor, 16, "Synthetic graph degree factor");
DEFINE_int32(gen_method, 0, "Synthetic graph generation method");
DEFINE_bool(gen_weights, false, "Generate synthetic edge weights");
DEFINE_int32(gen_weight_range, 255, "Synthetic weight range");

// Engine geometry.
DEFINE_int32(SEGMENT, 32, "Number of graph segments");
DEFINE_int32(n_stream, 4, "Number of CUDA streams");
DEFINE_int32(max_iteration, 1000, "Maximum iteration count");
DEFINE_int32(hybrid, 1, "Hybrid transfer enable (0=off)");
DEFINE_int32(residence, 1, "Residence re-execution enable");
DEFINE_int32(priority_a, 0, "Priority scheduling enable");
DEFINE_double(alpha, 0.8, "Cost-model alpha");
DEFINE_double(edge_factor, 1.0, "Edge factor for workload accounting");
DEFINE_string(lb_push, "", "Push load-balancing override");
DEFINE_string(lb_pull, "", "Pull load-balancing override");
DEFINE_string(out_wl, "", "Worklist output path");
DEFINE_bool(undirected, false, "Treat graph as undirected");
DEFINE_bool(wl_sort, false, "Sort worklists");
DEFINE_bool(wl_unique, false, "Deduplicate worklists");
DEFINE_double(wl_alloc_factor,   1.0,          "Worklist allocation factor");

// Diagnostics.
DEFINE_int32(block_size, 256, "CUDA block size");
DEFINE_int32(prio_delta, 0, "Priority delta override");
DEFINE_bool(check, false, "Run host reference check");
DEFINE_bool(verbose, false, "Verbose output");
DEFINE_bool(trace, false, "Trace output");
DEFINE_bool(stats, false, "Stats-only mode");
DEFINE_bool(estimate, false, "Policy estimation mode");
DEFINE_string(output, "", "Output result path");
// Cost-model flags used by policy::PolicyDecisionMaker.
DEFINE_double(beta,             0.40,         "Cost-model beta (ExpTM-Compaction vs ImpTM-Zero-Copy threshold)");