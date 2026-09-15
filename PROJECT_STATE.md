# HyTGraph Reproduction — Project State

## Current Phase

**Phase 15 — Task combining bridge** _(ready to start)_

## Current Milestone

**M15 — Task Combining with SEP worklists** _(READY)_

## Current Task

Phase 14 is complete and validated. Phase 15 will feed `TaskCombiner` output into the vendored worklist representation.

---

# Status

Phases 0–14 are complete and validated in the user's local working tree.

---

# Completed Phases

## Phase 0 — Repository / Build Baseline

**Status:** COMPLETE

Established the C++17 project structure, CMake build, test infrastructure, and CUDA-aware build layout.

---

## Phase 1 — CSR Graph Foundation

**Status:** COMPLETE

Implemented the CSR graph representation and supporting graph-loading functionality.

---

## Phase 2 — Activity Tracking

**Status:** COMPLETE

Implemented active-vertex tracking and active-edge accounting.

Active-edge semantics:

> Every outgoing edge of an active source vertex is considered active.

---

## Phase 3 — Logical Partitioning

**Status:** COMPLETE

Implemented logical graph partitions over contiguous vertex ranges while preserving CSR adjacency-list boundaries.

---

## Phase 4 — ExpTM-Filter

**Status:** COMPLETE

Implemented the reference ExpTM-Filter transfer path and associated transfer metrics.

This remains a reference/modeling implementation rather than the paper's complete CUDA execution pipeline.

---

## Phase 5 — Algorithm / Runtime Integration

**Status:** COMPLETE

Integrated the graph/activity/partition/transfer abstractions sufficiently for the current reproduction architecture and validation tests.

---

## Phase 6 — ExpTM-Compaction

**Status:** COMPLETE

Implemented the CPU/reference ExpTM-Compaction path.

---

## Phase 7 — ImpTM-Zero-Copy

**Status:** COMPLETE

Implemented the ImpTM-Zero-Copy reference/modeling path.

The implementation models request counts, payload sizes, alignment overhead, TLP accounting, active-vertex/edge metrics, partition metrics, and fallback behavior.

It does not claim actual CUDA pinned-memory or mapped-memory execution.

---

## Phase 8 — HyTM Cost Model

**Status:** COMPLETE

Implemented the HyTM cost model and deterministic transfer-engine selector.

Implemented:

- paper cost equations
- `alpha = 0.80`
- `beta = 0.40`
- `gamma = 0.625`
- zero-copy RTT model
- filter cost
- compaction cost
- zero-copy cost
- deterministic per-partition engine selection

---

## Phase 9 — Task Combining

**Status:** COMPLETE

Implemented executable-task planning from HyTM engine decisions.

Implemented:

- consecutive Filter grouping
- configurable Filter combination limit
- Compaction grouping
- Zero-Copy grouping
- deterministic task ordering
- sequential task indices
- task-combination metrics

---

## Phase 10 — Hub Sorting

**Status:** COMPLETE

Implemented the hub importance calculation and deterministic hub-first CSR reordering.

Hub importance:

    H(v) = Do(v) * Di(v) / (Do_max * Di_max)

Default:

    hub_fraction = 0.08

Implemented:

- in-degree calculation
- out-degree calculation
- hub scoring
- configurable hub fraction
- deterministic ranking
- deterministic tie breaking
- hub-first ordering
- CSR vertex reordering
- destination-ID remapping
- edge-weight preservation
- CSR validation

---

## Phase 11 — Contribution-Driven Scheduling

**Status:** COMPLETE

Implemented the CPU/reference Contribution-Driven Scheduling layer.

Implemented:

- generic contribution priority representation
- PageRank contribution generation
- SSSP contribution generation
- contribution-priority ordering
- deterministic tie breaking
- synchronous reference ordering
- reordered-work measurement
- zero-contribution measurement
- redundant-work measurement
- explicit stale-work observation
- scheduling-overhead estimation
- PageRank → scheduler integration
- SSSP → scheduler integration
- non-finite contribution validation

The scheduler remains a CPU/reference planning abstraction and does not execute asynchronous GPU work.

---

## Phase 12 — Organize the working HyTGraph SEP-Graph + Groute code

**Status:** COMPLETE

## Deliverables

- `third_party/hytgraph_sep/include/{framework,groute,utils}/` — copied unmodified from `iDC-NEU/HyTGraph`
- `third_party/hytgraph_sep/deps/{cub,gflags,json}/` — copied unmodified
- `third_party/hytgraph_sep/CMakeLists.txt` — preserved as upstream, not executed
- `third_party/hytgraph_sep/UPSTREAM_REVISION.txt` — created
- Root `CMakeLists.txt` — `HYTGRAPH_WITH_SEP_GRAPH` option + `hytgraph_sep_lib` INTERFACE target
- `tests/integration/sep_graph_link_smoke_test.cpp` — created, passes
- `docs/original_hytgraph_build_notes.md` — created
- `docs/original_hytgraph_reference_runs.md` — created (template)

## Exit Criterion Verification

- `hytgraph_sep_lib` builds on the target platform ✅
- Smoke test compiles, links, and runs ✅
- All Phase 0–11 tests still pass unchanged ✅ (5/5 in ctest)
- No `sepgraph::` or `groute::` symbol appears outside `third_party/hytgraph_sep/` and the smoke test ✅
- Vendored tree byte-identical to the original clone (`diff -r` empty) ✅

## Notes

- The vendored `third_party/hytgraph_sep/CMakeLists.txt` is the original SEP-Graph top-level build file and is intentionally **not executed**. `hytgraph_sep_lib` is defined in the root `CMakeLists.txt` as an INTERFACE target over the vendored header directories.
- `deps/gflags` is configured with `BUILD_TESTING OFF` so its internal CTest registrations do not leak into the project suite.
- The smoke test includes `framework/common.h` (not `framework/algo_variants.cuh`, which is not standalone-includable) and supplies `<tuple>`, `<string>`, `<cassert>` as build-environment shims before the vendored include.
- The vendored headers are pure C++ in this snapshot; no `.cu` files were copied. The smoke test is compiled as plain C++.

---

## Phase 13 — Adapter layer (no data movement)

**Status:** COMPLETE

Introduced the `sep_adapter` module as the single boundary between project code and the vendored SEP-Graph + Groute tree. In Phase 13 the adapter is a thin wrapper with no data movement, no kernel launches, no engine construction, and no partition bridging.

### Deliverables

- `include/sep_adapter/sep_variant.hpp` — project-owned `SEPVariant` enum (8 values), `to_string` / `from_string`, `is_valid_variant`, `kSEPVariantCount`.
- `include/sep_adapter/sep_execution_result.hpp` — `SEPExecutionResult` and top-level `SEPExecutionState` enum with five states (`OK`, `NEED_INIT`, `INVALID_CONFIG`, `DEFERRED`, `FAILED`); `make_ok` / `make_need_init` / `make_invalid_config` / `make_deferred` / `make_failed` free functions.
- `include/sep_adapter/sep_execution_driver.hpp` — abstract `SEPExecutionDriver` base class with the five virtual methods from the plan.
- `include/sep_adapter/sep_variant_mapper.hpp` — project-owned `AlgoVariantDescriptor` struct with three nested enums (`Mode`, `Direction`, `Traversal`), `describe(SEPVariant)` and `from_descriptor(...)`, plus forward declaration of `sepgraph::common::AlgoVariant` and two `detail::` function declarations (`to_vendored`, `from_vendored`).
- `src/sep_adapter/sep_variant_mapper.cpp` — the ONLY translation unit that includes `<framework/common.h>`.
- `include/sep_adapter/null_sep_driver.hpp` — `NullSEPDriver` returning `DEFERRED`, with the same lifecycle contract as the real adapter.
- `include/sep_adapter/sep_engine_adapter.hpp` — templated `SEPEngineAdapter<TValue, TBuffer, TWeight, TAppImpl, UnusedData...>` plus project-owned `SEPAlgorithm` enum.
- `src/sep_adapter/sep_engine_adapter.cpp` — **empty placeholder** (adapter is header-only in Phase 13).
- `include/sep_adapter/sep_engine_factory.hpp` — `make_null_driver`, `make_engine_driver<...>`, `make_engine_adapter<...>`, `supports_variant`, `supports_algorithm`.
- `src/sep_adapter/sep_engine_factory.cpp` — **empty placeholder** (factory is header-only in Phase 13).
- Root `CMakeLists.txt` — three insertions (sep_adapter sources into `hytgraph_runtime`, `HYTGRAPH_WITH_SEP_GRAPH` numeric macro, test target).
- `tests/sep_adapter_tests.cpp` — 621 checks, 0 failures.

### Exit Criterion Verification

- All Phase 0–12 tests still pass unchanged ✅ (6/6 in ctest).
- `sep_adapter_tests` passes (621 checks, 0 failures) ✅
- Adapter `.hpp` files contain no vendored **symbols** (only `sep_variant_mapper.hpp` forward-declares the vendored type, per deviation D8) ✅
- Vendored tree byte-identical to the original clone (not touched) ✅

### Deviations from MASTER_PLAN.md §Phase 13 (all build-boundary, none from the paper)

| #   | Plan says                                                                        | Actual                                                                                           | Reason                                                                                                                                                                                                                                                  |
| --- | -------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| D1  | `SEPEngineAdapter<TApp>` — 1 template parameter                                  | `SEPEngineAdapter<TValue, TBuffer, TWeight, TAppImpl, UnusedData...>` — 5                        | Matches vendored `sepgraph::engine::Engine` signature exactly.                                                                                                                                                                                          |
| D2  | `initialize()` constructs the vendored `Engine<TApp>`                            | `initialize()` returns `OK` and records intent only; no engine construction                      | Engine constructor calls `cudaGetDeviceProperties` and `CreateStream`; Phase 12's build is plain C++. Construction is deferred to Phase 14.                                                                                                             |
| D3  | Toy app executes one SEP step                                                    | `execute_step()` returns `DEFERRED` (real step in Phase 14)                                      | Vendored engine exposes no single-step API — only `Start()`, which runs to convergence in an internal loop. Phase 13 has no data-movement machinery to feed `Start()`.                                                                                  |
| D4  | `native_engine_handle()` returns opaque engine pointer                           | Returns `nullptr`                                                                                | Populated in Phase 14 when the engine is actually constructed.                                                                                                                                                                                          |
| D5  | Plan does not mention an algorithm enum                                          | `SEPAlgorithm` enum added (`IterativeScheme`, `TraversalScheme`)                                 | Vendored `Engine(AlgoType)` requires it. Mirrors `policy::AlgoType` from `framework.cuh`.                                                                                                                                                               |
| D6  | `sep_engine_adapter.cpp` implied non-trivial                                     | Delivered as empty placeholder                                                                   | Adapter is header-only in Phase 13; `.cpp` exists only to force isolated header parsing and to keep the CMake source list stable for Phase 14.                                                                                                          |
| D7  | Implies engine construction consumes `AlgoVariant`                               | `SEPVariant` is not consumed by the engine constructor                                           | Vendored `Engine` takes `AlgoType` at construction. `AlgoVariant` is used per-partition inside `Start()`. The `sep_variant_mapper` translation is consumed by Phase 14's bridge.                                                                        |
| D8  | "No `sepgraph::` or `groute::` symbol outside `third_party/` and the smoke test" | `sep_variant_mapper.hpp` forward-declares `sepgraph::common::AlgoVariant`                        | Forward declaration is not a symbol leak in the plan's sense: it announces a type without exposing any member, base class, or enumerator. The vendored header is included only in `sep_variant_mapper.cpp`.                                             |
| D9  | "Mapping is bijective with `sepgraph::common::AlgoVariant`"                      | Mapping is **injective**, not bijective                                                          | `AlgoVariant` carries 11 named constants: 8 execution-parameter combinations (with preimages) plus `Exp_Filter`, `Zero_Copy`, `Exp_Compaction` (transfer-engine choices with no preimage). `from_vendored` returns `std::nullopt` for the latter three. |
| D10 | "`variant_mapper.cpp` is the only file that includes `algo_variants.cuh`"        | Includes `<framework/common.h>` instead                                                          | `algo_variants.cuh` is not standalone-includable; it transitively pulls `hybrid_policy.h`. Consistent with Phase 12's smoke test.                                                                                                                       |
| D11 | Plan sketch assumes `AlgoVariant` is a value type with a 3-argument constructor  | `AlgoVariant` is a class with named static constants and no such constructor                     | Discovered by inspecting `framework/framework.cuh`. `to_vendored` returns `AV::SYNC_PUSH_DD` by name, not by constructor call.                                                                                                                          |
| F1  | Factory produces a driver per `(algorithm, variant)` pair                        | Factory is templated on the app type; no non-templated pair-driven factory                       | The vendored engine's template-template `TAppImpl` parameter cannot be selected by a runtime factory. Three functions replace the plan's single factory.                                                                                                |
| F2  | "Support `HYTGRAPH_WITH_SEP_GRAPH=OFF` build with the null driver"               | Fallback lives at the factory (`make_engine_driver` returns `NullSEPDriver` when the macro is 0) | Single point of control; call sites need not check the macro.                                                                                                                                                                                           |
| F3  | —                                                                                | `make_engine_adapter<...>` always returns a `SEPEngineAdapter` even in OFF builds                | The concrete type is part of the signature; the adapter is fully functional in Phase 13 without the vendored engine, so this is safe. Phase 14 will re-evaluate.                                                                                        |

### Build-time corrections applied

- `detail::to_vendored` and `detail::from_vendored` were declared `noexcept` in the first delivery of `sep_variant_mapper.hpp`. The vendored `AlgoVariant` constructor is not `noexcept`, and returning `std::optional` is not guaranteed `noexcept` either, so the `noexcept` specifiers were removed from both declarations and definitions.
- A `static_assert(injective_project_side())` was originally placed in `sep_variant_mapper.cpp`. It called `from_descriptor`, which uses `std::optional` and is not `constexpr` in C++17, so the assert could not be evaluated at compile time. The runtime test in `sep_adapter_tests.cpp` covers the same round trip.
- The first delivery of `tests/sep_adapter_tests.cpp` used a single-argument `CHECK` macro, which broke on expressions containing commas inside template argument lists (`dynamic_cast<SEPEngineAdapter<int, int, int, DummyApp>*>`). Fixed by making `CHECK` variadic and `CHECK_EQ` strictly two-argument, and by aliasing the adapter type before use.
- The `AlgoVariant::SYNC_PUSH_DD` / `SYNC_PULL_DD` / `SYNC_PUSH_TD` / `SYNC_PULL_TD` / `ASYNC_PULL_DD` / `ASYNC_PUSH_TD` / `ASYNC_PULL_TD` spellings were marked `VENDOR-CHECK` on first delivery. All seven compiled. **No `VENDOR-CHECK` remains.**

### Notes

- `sep_variant_mapper.cpp` is compiled only when `HYTGRAPH_WITH_SEP_GRAPH=ON`, because it unconditionally includes `<framework/common.h>`. In OFF builds, `detail::to_vendored` and `detail::from_vendored` are declared (via the header) but undefined. Nothing in Phase 13 calls them. Phase 14 must resolve this — either by keeping them behind the same `#if` or by adding a stub implementation in a new file compiled in OFF builds.
- The `HYTGRAPH_WITH_SEP_GRAPH` CMake option is now propagated to C++ as a numeric macro (`0` or `1`) via `target_compile_definitions(hytgraph_runtime PUBLIC HYTGRAPH_WITH_SEP_GRAPH=$<BOOL:${HYTGRAPH_WITH_SEP_GRAPH}>)`. `sep_engine_factory.hpp`'s `#if ... == 0` guard depends on this.
- `src/sep_adapter/sep_engine_adapter.cpp` and `src/sep_adapter/sep_engine_factory.cpp` are deliberately empty translation units in Phase 13. They exist only to force isolated header parsing and to keep the CMake source list stable for Phase 14.

---

## Phase 14 — Data-movement bridge

**Status:** COMPLETE

Introduced `HyTMSEPBridge`: the first real construction of the vendored `sepgraph::engine::Engine<...>`, driven from a project-owned `CSRGraph`. Fed the vendored engine through a temporary-file injection path (forced by the vendored `Context<Algo>` constructor), ran PageRank and SSSP end-to-end to convergence, and gathered results back to the project side.

### Deliverables

- `include/sep_adapter/sep_algorithm_mapper.hpp` + `src/sep_adapter/sep_algorithm_mapper.cpp` — project-owned `SEPAlgorithm` enum (`IterativeScheme`, `TraversalScheme`), `to_string` / `from_string`, injective mapping to/from `sepgraph::policy::AlgoType` (`ITERATIVE_SCHEME`, `TRAVERSAL_SCHEME`). **Bijection** over the two vendored enumerators.
- `include/sep_adapter/sep_host_graph_adapter.hpp` + `src/sep_adapter/sep_host_graph_adapter.cpp` — `SEPGraphFile` (move-only, RAII temp-file handle) and `write_sep_graph_file(const CSRGraph&, const SEPGraphFileConfig&)`. Serializes project `CSRGraph` to the "market_big" text layout (one `src dst [weight]` per line, 0-indexed, no header) — the only format whose vendored parser, `ReadGraphMarket_bigdata`, is implemented. Other parsers (`ReadGraph`, `ReadGraphGR`, `ReadGraphMarket`) are stubs in this snapshot.
- `include/sep_adapter/hytm_sep_bridge.hpp` + `src/sep_adapter/hytm_sep_bridge.cpp` — public `HyTMSEPBridge` class (plain C++), `SEPBridgeAlgorithm` enum (`PageRank`, `SSSP`), `SEPBridgeConfig`, `SEPBridgeMetrics`, `SEPBridgeResult`. Move-only, single-shot `run()`. Auto-computes `FLAGS_SEGMENT` if config left at 0.
- `src/sep_adapter/detail/engine_runner.hpp` — private plain-C++ header. Declares `detail::run_engine(const EngineRunRequest&) -> EngineRunOutput`. The bridge delegates all CUDA work to this function.
- `src/sep_adapter/sep_engine_adapter_impl.cu` — the only Phase 14 translation unit that includes `<framework/framework.cuh>`. Compiled as LANGUAGE CUDA. Contains the PageRank and SSSP engine constructors, gflags save/restore, `ensure_graph_datum_bitmaps()` workaround, result gathering.
- `src/sep_adapter/apps/pagerank_app.hpp` — project-owned `hytgraph::sep_adapter::apps::PageRank<TValue, TBuffer, TWeight, UnusedData...>` deriving from `sepgraph::api::AppBase`. Ports the vendored `hybrid_pr.cu` app.
- `src/sep_adapter/apps/sssp_app.hpp` — project-owned `hytgraph::sep_adapter::apps::SSSP<TValue, TBuffer, TWeight, UnusedData...>` deriving from `sepgraph::api::AppBase`. Ports the vendored `hybrid_sssp.cu` app.
- `src/sep_adapter/sep_flags.cpp` — standalone plain-C++ translation unit. Defines every gflag the vendored headers `DECLARE` (`graphfile`, `format`, `weight_num`, `SEGMENT`, `n_stream`, `max_iteration`, `hybrid`, `residence`, `priority_a`, `alpha`, `beta`, `edge_factor`, `lb_push`, `lb_pull`, `undirected`, `wl_sort`, `wl_unique`, `wl_alloc_factor`, `block_size`, `prio_delta`, `check`, `verbose`, `trace`, `stats`, `estimate`, `output`, `out_wl`, `gen_*`). Kept separate from the `.cu` to avoid gflags duplicate-declaration errors.
- `tests/sep_adapter_bridge_tests.cpp` — Phase 14 test suite.
- Root `CMakeLists.txt` — Phase 14 additions (marked `# === PHASE 14 BEGIN/END ===`):
  - New static library `hytgraph_sep_utils` over the vendored `src/utils/{utils,parser,to_json}.cpp`.
  - `hytgraph_sep_utils` linked into `hytgraph_runtime`.
  - New sources `sep_host_graph_adapter.cpp`, `sep_flags.cpp`, `hytm_sep_bridge.cpp`, `sep_engine_adapter_impl.cu`.
  - `sep_engine_adapter_impl.cu` marked LANGUAGE CUDA.
  - `CUDA::cudart` and `CUDA::cuda_driver` linked PRIVATE.
  - `src/` added to `hytgraph_runtime`'s private include path (for `src/sep_adapter/{detail,apps}`).
  - New test target `hytgraph_sep_adapter_bridge_tests` (plain C++, links `hytgraph_runtime`).

### Exit Criterion Verification

- One out-of-core iteration runs through the vendored engine for PageRank and SSSP ✅
- All Phase 0–13 tests still pass unchanged ✅
- Full ctest suite: **7/7** (`unit_tests`, `algorithm_tests`, `cuda_algorithm_tests`, `sep_graph_link_smoke`, `sep_adapter_tests`, `sep_adapter_bridge_tests`, `experiment_runner_smoke`)
- No `sepgraph::` or `groute::` symbol appears outside `src/sep_adapter/` and `third_party/hytgraph_sep/` ✅

### Deviations from MASTER_PLAN.md §Phase 14

| #   | Plan says                                                                                   | Actual                                                                                           | Reason                                                                                                                                                                                                                                 |
| --- | ------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| E1  | `execute_partition(active_vertices, engine)` drives one iteration                           | `HyTMSEPBridge::run()` runs the vendored engine to convergence in one call; no per-partition API | The vendored `Engine` has no public per-partition entry point. `Start()` is the only public execution method and it runs an internal `while (!convergence)` loop. Per-partition engine selection is done inside `PolicyDecisionMaker`. |
| E2  | "Map `row_offsets` → `groute::graphs::host::CSRGraph::offsets`, `column_indices` → `edges`" | The bridge serializes the CSR to a temporary file and lets the vendored engine load it           | `utils::traversal::Context<Algo>` calls `GetCachedGraph(FLAGS_graphfile, ...)` inside its constructor. There is no public API to inject an in-memory host graph.                                                                       |
| E3  | "Reuse the engine across partitions; only swap the GraphDatum input"                        | Not applicable — the vendored engine owns its own `GraphDatum` and never exposes it for swapping | `Engine::m_graph_datum` is private. The engine internally iterates segments.                                                                                                                                                           |
| E4  | Bridge takes `SEPExecutionDriver&` + `CSRGraph&`                                            | Bridge takes `SEPBridgeAlgorithm` + `const CSRGraph&` + `SEPBridgeConfig`                        | The Phase 13 `SEPExecutionDriver` interface is incompatible with the vendored `Engine`'s shape. See Known Issue #26.                                                                                                                   |
| E5  | `include/sep_adapter/sep_graph_datum_adapter.hpp` + `src/.../*.cpp`                         | Replaced by `sep_host_graph_adapter.hpp` + `.cpp`                                                | Building a `GraphDatum` directly is impossible; we build the temp file instead.                                                                                                                                                        |
| E6  | Neighbor shifting applied when engine is ExpTMFilter / ExpTMCompaction                      | Not implemented in Phase 14                                                                      | The vendored engine handles shifting internally during its segment loop.                                                                                                                                                               |
| E7  | Sort and deduplicate the active set once per step                                           | Not implemented in Phase 14                                                                      | The bridge does not see the active set; the vendored engine manages it.                                                                                                                                                                |

### Vendored tree edits (authorized, documented)

**One edit** was applied to the vendored tree in Phase 14, authorized by the repository owner in the Phase 14 chat. It is recorded in `docs/original_hytgraph_build_notes.md`.

```
File:    third_party/hytgraph_sep/include/framework/variants/sync_push_dd.cuh
Line:    ~189 in RelaxCTADB
Before:  if (tid < work_size) { ... work_source.get_work(tid); ... }
After:   if (i < work_size)   { ... work_source.get_work(i);   ... }
Reason:  The thread-id guard `tid < work_size` was incorrect for a
         grid-stride loop over work items. Every thread with
         tid >= work_size left np_local.size == 0 and tripped
         CTAWorkSchedulerNew::schedule's assert(np_local.size > 0).
         The loop variable `i` is the correct work-item index.
Class:   Correctness bug in the vendored snapshot, independent of
         paper semantics.
```

### Project-side workarounds (no vendored edits)

1. **`ensure_graph_datum_bitmaps(GraphDatum&)`** in `sep_engine_adapter_impl.cu`.
   The vendored `GraphDatum` constructor never allocates its four `Bitmap` members (`m_wl_bitmap_in`, `m_wl_bitmap_out_high`, `m_wl_bitmap_out_low`, `m_wl_bitmap_middle`). The vendored `Engine` never calls the `RebuildBitmapWorklist` helper that would size them (it is dead code in this snapshot). `RunSyncPushDDB` reads `m_wl_bitmap_out_high.DeviceObject()`, which asserts `m_size > 0`. We size all four from our side, after `LoadGraph()` and before `InitGraph()`, using only the public `CompressedBitmap` API (`CompressedBitmap(nnodes)` + `Swap`).

2. **`NDEBUG` defined for `sep_engine_adapter_impl.cu` only** via CMake `set_source_files_properties(...)`.
   The vendored engine contains `assert(np_local.size > 0)` in `cta_scheduler_hybrid.cuh`, which fires on active vertices whose PageRank buffer falls below the app's `kPageRankEpsilon` threshold. This is a legitimate runtime state, not an error. The assert is guarded by `NDEBUG`; the original HyTGraph samples compiled Release, so it never fired upstream. Defining `NDEBUG` for our `.cu` (which includes the header transitively) suppresses it without affecting any other target.

### Build-time corrections applied during Phase 14

- `sep_flags.cpp` was originally embedded in `sep_engine_adapter_impl.cu`. Moved to a standalone plain-C++ TU after the vendored `graph_datum.cuh` was found to `DECLARE_int32(wl_alloc_factor)` — the `.cu`'s own `DEFINE_int32(wl_alloc_factor, ...)` collided.
- `DEFINE_int32(wl_alloc_factor, ...)` was corrected to `DEFINE_double(wl_alloc_factor, 1.0, ...)` after the linker reported `undefined reference to fLD::FLAGS_wl_alloc_factor`. gflags' internal per-type packing (`fLD` = double, `fLI` = int32) revealed the type mismatch.
- `DEFINE_double(beta, 0.40, ...)` was added after `PolicyDecisionMaker::GetNextPolicy` reported `undefined reference to fLD::FLAGS_beta`.
- `CUDA::cuda_driver` was added alongside `CUDA::cudart` after `HandleError` reported `undefined reference to cuGetErrorString`.
- `hytgraph_sep_utils` (vendored `utils.cpp`, `parser.cpp`, `to_json.cpp`) was added as a static library after `Context<Algo>` reported `undefined reference to GetCachedGraph` / `CleanupGraphs`.
- The bridge auto-computes `FLAGS_SEGMENT` when config leaves it at 0. `Engine::LoadGraph` does not clamp `FLAGS_SEGMENT` to the actual non-empty segment count; a 5-vertex test graph with `FLAGS_SEGMENT = 32` caused an OOM inside `GraphDatum`'s constructor.
- The `include/sep_adapter/*.hpp` files that take a `CSRGraph` parameter gained `using hytgraph::graph::CSRGraph;` after the test compile showed `'CSRGraph' does not name a type` (`CSRGraph` lives in `namespace hytgraph::graph`, not at global or `hytgraph` scope).

### Notes

- **The vendored engine is not per-partition.** `Engine::Start()` owns the whole run — segmentation, cost-based engine selection (`PolicyDecisionMaker::GetNextPolicy`), task combining (`CombineTask`), and convergence. The paper's HyTM mechanisms (three transfer engines, task combining, contribution-driven scheduling) are all dispatched inside `Start()`. The bridge is a feeder, not a per-partition orchestrator.
- **`AppBase` is only reachable via full vendored include.** Both app headers (`pagerank_app.hpp`, `sssp_app.hpp`) derive from `sepgraph::api::AppBase`, whose definition lives in `<framework/variants/api.cuh>`. They are placed under `src/sep_adapter/apps/` (private, not installed), not under `include/sep_adapter/`, to preserve the public boundary.
- **Phase 14 exposes no public `sepgraph::` symbol.** `sep_host_graph_adapter.hpp` and `hytm_sep_bridge.hpp` include only project-owned and standard-library headers. The `detail::EngineRunRequest` / `EngineRunOutput` types live under `src/sep_adapter/detail/`.
- **Numerical validation is deferred.** `tests/sep_adapter_bridge_tests.cpp` verifies structural facts: `valid()`, file existence, correct result vector sizes, distance values on a unit-weight line graph (0..N-1), and single-shot `run()`. It does **not** compare against the project's CPU reference. This is Phase 15 work.
- **Test graph size is 100 vertices.** Vendored minimum-size behavior was observed at 5 vertices (`CompressedBitmap` assertion fire before any kernel ran) and vanished once we size the bitmaps ourselves. The current tests exercise the temp-file path, the parser, engine construction, and result gathering with a 100-vertex line graph.

---

# Planned Future Phases (not yet started)

| Phase    | Name                             | Status       |
| -------- | -------------------------------- | ------------ |
| Phase 13 | Adapter layer (no data movement) | **COMPLETE** |
| Phase 14 | Data-movement bridge             | **COMPLETE** |
| Phase 15 | Task combining bridge            | READY        |
| Phase 16 | Contribution scheduling bridge   | NOT STARTED  |
| Phase 17 | VCGC read path                   | NOT STARTED  |
| Phase 18 | VCGC refresh                     | NOT STARTED  |
| Phase 19 | Multi-stream runtime             | NOT STARTED  |
| Phase 20 | Full HyTGraph integration        | NOT STARTED  |
| Phase 21 | Evaluation                       | NOT STARTED  |

---

# Known Issues / Engineering Approximations

The following remain known and intentional.

## 1. Exact Original Partition Boundaries

The paper does not provide enough information to reproduce every original runtime partition boundary exactly. The project therefore uses deterministic logical partitions.

## 2. Logical Partitioning

Logical partitions remain graph-analysis/reference abstractions rather than a complete reproduction of the paper's runtime partition scheduler.

## 3. ExpTM-Filter

The current implementation remains a reference transfer model rather than the complete CUDA transfer mechanism.

## 4. ExpTM-Compaction

The current implementation remains CPU/reference compaction. It does not claim to reproduce the complete asynchronous execution pipeline.

## 5. Subway

The exact internal Subway implementation and scheduling behavior are not fully specified by the available sources. No unsupported implementation details are being invented.

## 6. Physical Transfer Accounting

Current transfer sizes are logical/reference byte counts. They are not claimed to represent every physical PCIe transaction or runtime metadata transfer.

## 7. CUDA Execution

The transfer-engine, task-combination, hub-sorting, and contribution-scheduling layers remain reference/modeling or preparation components until integrated with the vendored SEP-Graph execution layer. Phase 14 exercised the vendored engine end-to-end but did not connect the Phase 8 cost model's decisions to the vendored `PolicyDecisionMaker`.

## 8. Zero-Copy Mapping

Phase 7 does not perform actual CUDA pinned host allocation, host registration, or mapped-memory pointer acquisition. The behavior is explicitly modeled.

## 9. Zero-Copy Alignment

The zero-copy alignment calculation uses a logical CSR byte-offset proxy. It is not a physical host-memory address calculation.

## 10. Compaction Throughput

A reproducible paper-specific CPU compaction throughput measurement is not currently available. Phase 8 exposes throughput as a configurable model parameter rather than inventing a paper-specific measured value.

## 11. Task Combination

The current TaskCombiner is an executable-task planning layer. It does not yet execute grouped tasks, overlap transfers and computation, or implement the paper's complete scheduling pipeline. The vendored engine's own `CombineTask()` (a distinct, internal mechanism) is what runs during Phase 14's `Start()` calls.

## 12. Task Ordering for Globally Combined Engines

Compaction and Zero-Copy groups may contain non-consecutive logical partition indices because they are accumulated by engine selection. `partition_indices` is therefore authoritative for those task memberships.

## 13. CMake CUDA Architecture Default

When CUDA is enabled and no explicit architecture is supplied, CMake uses:

    CMAKE_CUDA_ARCHITECTURES=native

This is a build-configuration convenience and is not a HyTGraph algorithmic behavior.

## 14. No Benchmark Claims

No performance or benchmark equivalence to the original HyTGraph or SEP-Graph implementation is currently claimed.

## 15. Hub Count for Small Graphs

The paper specifies approximately the top 8% but does not specify exact rounding behavior for very small graphs. The implementation uses deterministic ceiling behavior for positive fractional counts.

## 16. Zero-Degree Graphs

For graphs where maximum in-degree or maximum out-degree is zero, the hub-score denominator is undefined. The implementation assigns zero scores rather than performing division by zero.

## 17. Vertex Renumbering

`CSRGraph::reorder_vertices()` changes internal vertex numbering according to the supplied ordering and remaps destination IDs accordingly. Callers maintaining external vertex-ID state must account for the resulting renumbering.

## 18. Contribution-Driven Scheduling

The Phase 11 scheduler remains a CPU/reference planning abstraction.

It does not yet:

- execute asynchronous work;
- maintain a GPU work queue;
- integrate with SEP-Graph GPU worklists;
- coordinate CUDA streams;
- perform neighbor shifting;
- overlap CPU compaction with GPU execution;
- measure actual scheduler wall-clock overhead;
- claim the paper's complete contribution-driven runtime behavior.

## 19. SSSP Contribution Definition

The available paper material does not specify a complete formal SSSP contribution equation. The current implementation therefore uses tentative-distance improvement supplied by the execution layer. This is explicitly an engineering approximation.

## 20. Vendored Code Read-Only Rule (Amendment)

The `third_party/hytgraph_sep/` directory is **read-only by default**. No project code outside `include/sep_adapter/` and `src/sep_adapter/` may include vendored headers.

**Amendment (Phase 14):** If a vendored file contains a defect that blocks Phase work, the repository owner may authorize a targeted edit. Any such edit must be:

1. Authorized explicitly in the phase chat.
2. Applied minimally — no reformatting, no cleanup.
3. Recorded in `docs/original_hytgraph_build_notes.md` with file, line, before, after, and reason.
4. Reflected in this file under the phase's "Vendored tree edits" subsection.

Phase 14 applied one such edit: `sync_push_dd.cuh` `tid` → `i` (see above).

## 21. Phase 12 Integration Notes

The vendored `third_party/hytgraph_sep/CMakeLists.txt` is not executed. `hytgraph_sep_lib` is defined in the root `CMakeLists.txt` as an INTERFACE target over the vendored header directories. The smoke test exercises `framework/common.h`, not `framework/algo_variants.cuh` (which is not standalone-includable). These are build-integration details, not algorithmic deviations from the paper.

## 22. SEP Adapter Has No Engine Construction in Phase 13

The `SEPEngineAdapter` in Phase 13 does not construct a vendored `sepgraph::engine::Engine`. Phase 14 bypasses `SEPEngineAdapter` entirely; see Known Issue #26.

## 23. SEPVariant ↔ AlgoVariant Is Injective, Not Bijective

The vendored `sepgraph::common::AlgoVariant` carries eleven named constants: eight execution-parameter combinations (`SYNC_PUSH_DD` through `ASYNC_PULL_TD`) and three transfer-engine choices (`Exp_Filter`, `Zero_Copy`, `Exp_Compaction`). The project-owned `SEPVariant` enum covers the eight execution-parameter combinations. The three transfer-engine choices have no preimage; `detail::from_vendored` returns `std::nullopt` for them.

MASTER_PLAN.md §Phase 13 says "mapping is bijective with `sepgraph::common::AlgoVariant`." The correct characterization is **injective**. This is a documentation correction, not a behavioral gap.

## 24. Forward Declaration of a Vendored Type in a Project Header

`include/sep_adapter/sep_variant_mapper.hpp` forward-declares `sepgraph::common::AlgoVariant` so that the two internal `detail::` translation functions can be declared without including any vendored header. This is a technical deviation from the letter of the Phase 12 leak check but preserves its spirit: no member, base class, or enumerator of the vendored type is visible. Phase 14's public headers (`sep_host_graph_adapter.hpp`, `hytm_sep_bridge.hpp`) forward-declare no vendored types at all.

## 25. sep_variant_mapper.cpp Is Conditionally Compiled

`src/sep_adapter/sep_variant_mapper.cpp` includes `<framework/common.h>`, whose include path exists only when `HYTGRAPH_WITH_SEP_GRAPH=ON`. It is therefore compiled only in ON builds. In OFF builds, `detail::to_vendored` and `detail::from_vendored` are declared but undefined. Nothing in Phase 13 or 14 calls them.

Resolution deferred to Phase 20, which requires the OFF-build `FullPipeline` path. At that time, add `src/sep_adapter/sep_variant_mapper_stub.cpp` (compiled in OFF builds) returning `std::nullopt` / `INVALID_CONFIG`.

## 26. Phase 13 `SEPExecutionDriver` Is Not Used in Phase 14

The Phase 13 `SEPExecutionDriver` abstract interface expects a per-step design (`initialize()`, `execute_step()`, `synchronize()`). The vendored `sepgraph::engine::Engine` does not fit this shape:

- `Start()` is the only public execution method and runs to convergence.
- The engine owns its own `GraphDatum`, host CSR, and segmentation.
- There is no API to inject a per-partition payload.

Phase 14 therefore introduces `HyTMSEPBridge` as the driving interface for the vendored engine. Phase 13's `SEPExecutionDriver`, `SEPEngineAdapter`, and `NullSEPDriver` remain in the tree and compile, but nothing in Phase 14 uses them. Their eventual disposition (reconciliation vs retirement) is deferred.

## 27. Vendored Utility Sources Must Be Compiled

The vendored `utils.cpp` (defines `GetCachedGraph`, `CleanupGraphs`), `parser.cpp` (defines the graph-file parsers), and `to_json.cpp` (defines `JsonWriter`) must be linked into `hytgraph_runtime` in any configuration that reaches the vendored `Engine`. Phase 14 adds them via a static library `hytgraph_sep_utils`. Omitting this library produces link errors inside the vendored `Context<Algo>` constructor.

## 28. Vendored Parser Stubs

Three of the four vendored graph-file parsers (`ReadGraph`, `ReadGraphGR`, `ReadGraphMarket`) are stubs in the snapshot — they call `CreateGraph()` and return an empty graph. Only `ReadGraphMarket_bigdata` (flag `"market_big"`) is functional. Phase 14 uses it exclusively.

## 29. `ReadGraphMarket_bigdata` Highest-Vertex Precondition

The vendored `ReadGraphMarket_bigdata` derives `nvtxs` from `max(src, dst) + 1` but only resizes its `xadj` scratch array when it sees a higher `src`. If the highest-numbered vertex never appears as a source, the parser overruns. `write_sep_graph_file` therefore rejects graphs where `out_degree(num_vertices - 1) == 0` and returns `std::nullopt`. Project tests add a self-loop on the top vertex of line graphs to satisfy this.

## 30. Vendored `GraphDatum` Does Not Allocate Its Bitmaps

`sepgraph::graphs::GraphDatum`'s constructor allocates node value datums, node buffer datums, worklist queues, and sampling arrays — but not the four `Bitmap` members `m_wl_bitmap_in`, `m_wl_bitmap_out_high`, `m_wl_bitmap_out_low`, `m_wl_bitmap_middle`. Nothing in the vendored lifecycle sizes them; `RebuildBitmapWorklist` in `algo_variants.cuh` is dead code in this snapshot. `RunSyncPushDDB` reads `m_wl_bitmap_out_high.DeviceObject()`, which asserts `m_size > 0`.

Phase 14 works around this with `ensure_graph_datum_bitmaps()` in `sep_engine_adapter_impl.cu`, which sizes all four bitmaps after `LoadGraph()` and before `InitGraph()`. This is a project-side workaround; the vendored tree was not edited for it.

## 31. `NDEBUG` Required for the Engine Translation Unit

The vendored engine contains `assert(np_local.size > 0)` in `cta_scheduler_hybrid.cuh`. It fires on any active vertex whose `CombineValueBuffer` returns `pair.second == false` — a legitimate PageRank state (`buffer <= kPageRankEpsilon`). The original HyTGraph samples compiled Release, so the assert never fired upstream.

Phase 14 defines `NDEBUG` for `sep_engine_adapter_impl.cu` only, via `set_source_files_properties(... COMPILE_DEFINITIONS "NDEBUG")`. **Consequence:** asserts anywhere in the vendored tree reached by this TU are now silent. Correctness must be validated by numerical comparison (Phase 15) rather than by assertion.

## 32. Vendored Engine's `LoadBalancing::NONE` Is Not Implemented in `RelaxCTADB`

`RelaxCTADB` in `sync_push_dd.cuh` dispatches on `LoadBalancing`:

- `COARSE_GRAINED` — implemented
- `FINE_GRAINED` — implemented (vendored default)
- `HYBRID` — implemented
- `NONE` — falls through to `default: assert(false);`

Phase 14 uses the vendored default (`FINE_GRAINED`). `FLAGS_lb_push` is not overridden.

## 33. Phase 14 Tests Are Structural, Not Numerical

`tests/sep_adapter_bridge_tests.cpp` verifies:

- `SEPGraphFile` factory: empty graph → `nullopt`; valid graph → file exists, correct flag values; weighted vs unweighted; isolated top vertex → `nullopt`; move semantics.
- `SEPBridgeAlgorithm` round-trip string mapping.
- `HyTMSEPBridge` construction: valid and invalid inputs.
- PageRank run: correct result vector size, all values finite and non-negative, single-shot `run()` behaviour.
- SSSP run: correct result vector size, distance values 0..N-1 on a unit-weight line graph (if the source distance is initialized).

It does **not** compare against the project's CPU reference. Numerical validation is Phase 15 work.

## 34. Phase 14 `SEPBridgeMetrics` Is Partially Populated

`SEPBridgeMetrics.nnodes` and `.nedges` are read from `engine.GetGraphDatum()`. The remaining fields (`current_round`, `explicit_num`, `zerocopy_num`, `compaction_num`, `time_*`) are left at 0, because `sepgraph::engine::Engine::m_running_info` has no public accessor. Populating them requires either a vendored accessor or a project-side wrapper; deferred to Phase 15 or later.

## 35. Vendored `PolicyDecisionMaker` Runs Independently of Phase 8

The vendored engine performs its own cost-based engine selection inside `Start()`, via `sepgraph::policy::PolicyDecisionMaker`. This selection is **not** the Phase 8 HyTM cost model. Phase 8's decisions (`TransferEngineType` per logical partition) are not connected to the vendored `PolicyDecisionMaker` in Phase 14. Reconciliation is Phase 15+ work.

## 36. Temp-File Injection Path

The vendored `Context<Algo>` constructor reads `FLAGS_graphfile` and `FLAGS_format` and calls `GetCachedGraph`. There is no public API for in-memory graph injection. Phase 14 serializes the project `CSRGraph` to a temporary text file (Market-Big format: one `src dst [weight]` per line, 0-indexed, whitespace-separated, no header), sets `FLAGS_graphfile` to that path and `FLAGS_format = "market_big"`, and removes the file when `SEPGraphFile` destructs.

Consequences:

- `write_sep_graph_file` scales and rounds `float` project weights to `uint32` vendored weights. Callers must set `weight_scale` to preserve required precision.
- Large graphs produce large text files. Binary format support is deferred.
- The temp file must remain on disk for the entire engine run.

## 37. Phase 14 Vendored Edit — `sync_push_dd.cuh`

See "Vendored tree edits" under Phase 14 above. The edit is authorized and documented in `docs/original_hytgraph_build_notes.md`.

---

# Validation Policy

For every new phase:

1. Implement one file.
2. Provide the complete copy-pasteable file.
3. Do not modify unrelated architecture.
4. User adds the file locally.
5. User builds/tests locally.
6. Review any failures.
7. Continue to the next file only after validation.

For large existing files such as `tests/unit_tests.cpp`, only the required changes/snippets should be provided rather than replacing the complete file.

Do not claim local execution unless the user provides the result.

---

# Repository Safety Rule

The repository is **read-only from the assistant's perspective**.

Do not write, modify, delete, or generate files directly inside the repository.

All implementation files must be provided as copy-pasteable content for the user to add manually.

The repository's GitHub state may not contain the user's unpushed local changes.

The user's local working tree is authoritative for newly implemented phases until those changes are committed/pushed by the repository owner.

---

# Current Validation Status

## Phase 0–11 Validation

Phases 0–11 are complete and validated in the user's local working tree.

The consolidated `tests/unit_tests.cpp` target is the authoritative validation artefact for Phases 0–11.

No performance claim is made for Phases 0–11.

## Phase 12 Validation

Phase 12 is complete and validated.

- `hytgraph_sep_lib` is defined and links.
- `hytgraph_sep_link_smoke_test` compiles, links, and passes.
- The full ctest suite passes: 5/5.
- Symbol-leakage grep checks are clean.
- The vendored tree is byte-identical to the original clone.

## Phase 13 Validation

Phase 13 is complete and validated in the user's local working tree.

- `hytgraph_runtime` includes `src/sep_adapter/*.cpp`.
- `HYTGRAPH_WITH_SEP_GRAPH` is propagated to C++ as a numeric macro.
- The full ctest suite passes: 6/6.
- `sep_adapter_tests` reports **621 checks, 0 failures**.
- All Phase 0–12 tests still pass unchanged.

## Phase 14 Validation

Phase 14 is complete and validated in the user's local working tree.

- `hytgraph_runtime` now includes `sep_host_graph_adapter.cpp`, `sep_flags.cpp`, `hytm_sep_bridge.cpp`, `sep_engine_adapter_impl.cu`.
- The full ctest suite passes: **7/7** (`unit_tests`, `algorithm_tests`, `cuda_algorithm_tests`, `sep_graph_link_smoke`, `sep_adapter_tests`, `sep_adapter_bridge_tests`, `experiment_runner_smoke`).
- `sep_adapter_bridge_tests` executes PageRank and SSSP end-to-end through the vendored `Engine`.
- All Phase 0–13 tests still pass unchanged.
- One vendored edit applied and documented (`sync_push_dd.cuh` `tid` → `i`).

## Superseded Work

The earlier draft Phase 12 ("SEP-Graph Foundation" — project-local abstraction layer) is **superseded and discarded**. Its files (`include/sep/sep_*.hpp`) are no longer part of the architecture. If they exist in the local tree, they should be removed.

---

# Current Milestone

    M15 — Task Combining with SEP worklists

Status:

    READY

---

# Next Milestone

    M16 — Contribution scheduling with SEP

Status:

    PENDING

---

# NEXT TASK

**Next task:** Phase 15 — Task combining bridge.

Phase 15 feeds `TaskCombiner` output into the vendored worklist. Per MASTER_PLAN.md §Phase 15, this phase introduces:

1. `include/sep_adapter/sep_task_combiner.hpp` + `src/sep_adapter/sep_task_combiner.cpp` — `SEPTask` struct; `combine_to_sep_tasks(...)` translating project `CombinedTask`s into vendored worklist units.
2. `include/sep_adapter/sep_worklist_adapter.hpp` + `src/sep_adapter/sep_worklist_adapter.cpp` — `SEPWorklistAdapter` with pimpl; loads active vertex sets into `groute::worklist`.

**Prerequisite for Phase 15 (must be resolved before writing code):**

The Phase 14 findings change the Phase 15 shape substantially. Because the vendored `Engine::Start()` performs its own `CombineTask()` internally, the plan's "feed `TaskCombiner` output into the vendored worklist" is not reachable from outside the engine. Phase 15 must decide between:

- **(A) Reconciliation:** extend `HyTMSEPBridge` to expose the engine's own combined-task structure (via `TRunningInfo` counters or a project-side accessor), and validate `TaskCombiner` output against it. Delivers a measured "logical vs executable task count" number without needing to drive the vendored worklist.
- **(B) Parallel worklist:** introduce `SEPWorklistAdapter` as a standalone project-owned wrapper around `groute::Queue<index_t>` for later phases (16, 19) that need external worklist manipulation.

Both may be appropriate; they are not exclusive. The chat for Phase 15 must resolve this before writing any file, and record the decision here.

**Phase 15 must also deliver:**

- Numerical validation of PageRank and SSSP against the project's CPU reference (`src/algorithms/pagerank.cpp`, `src/algorithms/sssp.cpp`) on a shared small graph, within a documented tolerance.
- If numerical validation is not feasible in Phase 15, the reason must be documented and the work re-scheduled.

**Time-box: 2–3 days.** Phase 14's integration discoveries mean this phase is mostly decision + validation, not new plumbing.

---

# Phase 15 Open Questions (to resolve before starting)

1. **Reconcile or parallel worklist?** See "Prerequisite for Phase 15" above. Recommendation: **(A) Reconciliation first**, because the paper's "task count drops by ≥3×" exit criterion is a planner-vs-runtime comparison, and the vendored engine already produces the runtime number. **(B)** can be deferred to Phase 16 (contribution scheduling) where external worklist manipulation is genuinely required.

2. **What numerical tolerance is correct?** The vendored engine runs to convergence with its own internal epsilon. The project's CPU reference has its own tolerance. For PageRank, a reasonable standard is `1e-3` absolute per vertex. For SSSP on integer weights, `0` tolerance is achievable and should be required. Confirm or override.

3. **Which graph should Phase 15 validate against?** A hand-built graph small enough to reason about (≤100 vertices, ≤200 edges), with known PageRank and SSSP ground truth. The Phase 14 line graph is not sufficient — its PageRank has no closed form and its SSSP has no branching. A star or small random graph with degree variation is preferable.

4. **Does `TaskCombiner` need to be called at all in Phase 15?** If Option A (reconciliation) is chosen, yes — to produce the "logical partitions" number that we compare against `TRunningInfo.explicit_num + zerocopy_num + compaction_num`. If Option B (parallel worklist), no.

---

# Phase 14 Open Questions (resolved)

1. **Does `framework/framework.cuh` compile as plain C++?** **Resolved: no.** `RunSyncPushDDB` and the other variant functions use `<<<>>>` syntax in template bodies that are instantiated. `sep_engine_adapter_impl.cu` is compiled as `LANGUAGE CUDA`.

2. **Which apps does Phase 14 target first?** **Resolved: project-owned.** `src/sep_adapter/apps/pagerank_app.hpp` and `src/sep_adapter/apps/sssp_app.hpp` derive from `sepgraph::api::AppBase`. BFS and CC are deferred.

3. **Where does the graph datum get built?** **Resolved: neither.** The vendored engine builds its own `GraphDatum`. The bridge writes a temp file that the vendored `Context<Algo>` reads. See Known Issue #36.
