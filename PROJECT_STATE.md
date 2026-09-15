# HyTGraph Reproduction — Project State

## Current Phase

**Phase 14 — Data-movement bridge** _(ready to start)_

## Current Milestone

**M14 — HyTGraph ↔ SEP data-movement bridge** _(READY)_

## Current Task

Phase 13 is complete and validated. Phase 14 will feed the project-owned CSR graph and active-vertex set into the vendored SEP-Graph engine through `HyTMSEPBridge`, and will introduce the first real engine construction (`sepgraph::engine::Engine<...>`).

---

# Status

Phases 0–13 are complete and validated in the user's local working tree.

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

# Planned Future Phases (not yet started)

| Phase    | Name                             | Status       |
| -------- | -------------------------------- | ------------ |
| Phase 13 | Adapter layer (no data movement) | **COMPLETE** |
| Phase 14 | Data-movement bridge             | READY        |
| Phase 15 | Task combining bridge            | NOT STARTED  |
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

The transfer-engine, task-combination, hub-sorting, and contribution-scheduling layers remain reference/modeling or preparation components until integrated with the vendored SEP-Graph execution layer.

## 8. Zero-Copy Mapping

Phase 7 does not perform actual CUDA pinned host allocation, host registration, or mapped-memory pointer acquisition. The behavior is explicitly modeled.

## 9. Zero-Copy Alignment

The zero-copy alignment calculation uses a logical CSR byte-offset proxy. It is not a physical host-memory address calculation.

## 10. Compaction Throughput

A reproducible paper-specific CPU compaction throughput measurement is not currently available. Phase 8 exposes throughput as a configurable model parameter rather than inventing a paper-specific measured value.

## 11. Task Combination

The current TaskCombiner is an executable-task planning layer. It does not yet execute grouped tasks, overlap transfers and computation, or implement the paper's complete scheduling pipeline.

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

## 20. Vendored Code Read-Only Rule

The `third_party/hytgraph_sep/` directory is read-only from the project's perspective. No project code outside `include/sep_adapter/` and `src/sep_adapter/` may include vendored headers. No edits to vendored source files are permitted; if a build fix is required, it must be applied identically to `docs/original_hytgraph_build_notes.md` and reflected in the vendored tree only as documented.

## 21. Phase 12 Integration Notes

The vendored `third_party/hytgraph_sep/CMakeLists.txt` is not executed. `hytgraph_sep_lib` is defined in the root `CMakeLists.txt` as an INTERFACE target over the vendored header directories. The smoke test exercises `framework/common.h`, not `framework/algo_variants.cuh` (which is not standalone-includable). These are build-integration details, not algorithmic deviations from the paper.

## 22. SEP Adapter Has No Engine Construction in Phase 13

The `SEPEngineAdapter` in Phase 13 does not construct a vendored `sepgraph::engine::Engine`. The engine's constructor calls `cudaGetDeviceProperties` and `CreateStream`, which would fail on any build environment without CUDA. Construction is deferred to Phase 14.

`execute_step()` returns `DEFERRED` in Phase 13 because the vendored engine exposes no single-step API. The engine's only public execution entry point is `Start()`, which runs the algorithm to convergence in an internal `while (!convergence)` loop. A project-side step scheduler will be introduced in Phase 14.

## 23. SEPVariant ↔ AlgoVariant Is Injective, Not Bijective

The vendored `sepgraph::common::AlgoVariant` carries eleven named constants: eight execution-parameter combinations (`SYNC_PUSH_DD` through `ASYNC_PULL_TD`) and three transfer-engine choices (`Exp_Filter`, `Zero_Copy`, `Exp_Compaction`). The project-owned `SEPVariant` enum covers the eight execution-parameter combinations. The three transfer-engine choices have no preimage; `detail::from_vendored` returns `std::nullopt` for them.

MASTER_PLAN.md §Phase 13 says "mapping is bijective with `sepgraph::common::AlgoVariant`." The correct characterization is **injective**. This is a documentation correction, not a behavioral gap.

## 24. Forward Declaration of a Vendored Type in a Project Header

`include/sep_adapter/sep_variant_mapper.hpp` forward-declares `sepgraph::common::AlgoVariant` so that the two internal `detail::` translation functions can be declared without including any vendored header. This is a technical deviation from the letter of the Phase 12 leak check ("no `sepgraph::` or `groute::` symbol appears outside `third_party/hytgraph_sep/` and the smoke test") but preserves its spirit: no member, base class, or enumerator of the vendored type is visible; the type remains incomplete in every translation unit that does not include `<framework/common.h>`.

If a stricter interpretation is required in a future phase, move the two `detail::` declarations to a private header under `src/sep_adapter/` that is not installed.

## 25. sep_variant_mapper.cpp Is Conditionally Compiled

`src/sep_adapter/sep_variant_mapper.cpp` includes `<framework/common.h>`, whose include path exists only when `HYTGRAPH_WITH_SEP_GRAPH=ON`. It is therefore compiled only in ON builds. In OFF builds, `detail::to_vendored` and `detail::from_vendored` are declared but undefined. Nothing in Phase 13 calls them.

Phase 14 must resolve this. Two options:

1. Keep them behind the same `#if` and never call them in OFF builds.
2. Add a stub implementation in a new file (`src/sep_adapter/sep_variant_mapper_stub.cpp`) compiled in OFF builds, returning failure / `std::nullopt`.

Option 2 is cleaner.

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
- The full ctest suite passes: 5/5 (`unit_tests`, `algorithm_tests`, `cuda_algorithm_tests`, `sep_graph_link_smoke`, `experiment_runner_smoke`).
- Symbol-leakage grep checks are clean.
- The vendored tree is byte-identical to the original clone.

## Phase 13 Validation

Phase 13 is complete and validated in the user's local working tree.

- `hytgraph_runtime` now includes `src/sep_adapter/*.cpp`.
- `HYTGRAPH_WITH_SEP_GRAPH` is propagated to C++ as a numeric macro.
- The full ctest suite passes: 6/6 (`unit_tests`, `algorithm_tests`, `cuda_algorithm_tests`, `sep_graph_link_smoke`, `sep_adapter_tests`, `experiment_runner_smoke`).
- `sep_adapter_tests` reports **621 checks, 0 failures**.
- All Phase 0–12 tests still pass unchanged.
- The vendored tree is byte-identical to the original clone.
- All seven previously-inferred `AlgoVariant::SYNC_*` / `ASYNC_*` names compiled without adjustment. **No `VENDOR-CHECK` remains in Phase 13 code.**

## Superseded Work

The earlier draft Phase 12 ("SEP-Graph Foundation" — project-local abstraction layer) is **superseded and discarded**. Its files (`include/sep/sep_*.hpp`) are no longer part of the architecture. If they exist in the local tree, they should be removed.

---

# Current Milestone

    M14 — HyTGraph ↔ SEP data-movement bridge

Status:

    READY

---

# Next Milestone

    M15 — Task Combining with SEP worklists

Status:

    PENDING

---

# NEXT TASK

**Next task:** Phase 14 — Data-movement bridge.

Phase 14 feeds the project-owned CSR graph and active-vertex set into the vendored SEP-Graph engine through a project-owned bridge, and introduces the first real engine construction.

Phase 14 deliverables:

1. `include/sep_adapter/sep_algorithm_mapper.hpp` + `src/sep_adapter/sep_algorithm_mapper.cpp` — project-owned mapping from `SEPAlgorithm` to the vendored `policy::AlgoType`.
2. `include/sep_adapter/sep_graph_datum_adapter.hpp` + `src/sep_adapter/sep_graph_datum_adapter.cpp` — build a `sepgraph::graphs::GraphDatum` from a project-owned `CSRGraph` plus an active-vertex set. pimpl so no vendored symbol leaks into public headers.
3. `include/sep_adapter/hytm_sep_bridge.hpp` + `src/sep_adapter/hytm_sep_bridge.cpp` — `HyTMSEPBridge` accepting a `SEPExecutionDriver&` and a `CSRGraph&`; `execute_partition(active_vertices, engine)` drives one iteration.
4. `src/sep_adapter/sep_engine_adapter_impl.cpp` (new, CUDA-enabled) — the actual engine construction. Either a `.cpp` compiled with CUDA support, or a `.cu` file. **This file will include `<framework/framework.cuh>`.**
5. Update `CMakeLists.txt` to add the new sources, and to compile `sep_engine_adapter_impl.cpp` as a CUDA source (or as a plain C++ source if the header is `.cuh`-compatible without kernel launches — this is a decision to make during Phase 14, not now).
6. Update `tests/sep_adapter_tests.cpp` to add a Phase 14 test section, or add a new `tests/sep_adapter_bridge_tests.cpp` if the bridge needs its own test target.
7. Correct the two Phase 13 documentation bugs: the "bijective" comment in `sep_variant_mapper.hpp`; the deferred OFF-build stub decision (Known Issue #25).

**Prerequisite for Phase 14 (must be resolved before writing code):**

- Read `framework/graph_datum.cuh` and confirm the `GraphDatum` constructor signature and the semantics of `m_current_round`, `m_wl_array_in_seg`, and `subgraphedges`.
- Read `framework/variants/api.cuh` and `framework/variants/driver.cuh` to understand `RunSyncPushDDB` and the other driver functions the engine calls internally.
- Decide whether Phase 14 builds `sep_engine_adapter_impl.cpp` as C++ or CUDA. The header `framework.cuh` uses `<<<>>>` syntax internally (kernel launches) in template bodies that are only instantiated when called; whether a `.cpp` can include it without nvcc depends on whether the template bodies reach `kernel::*` calls before instantiation. **Test this before writing the bridge.**

Exit criteria for Phase 14:

- One out-of-core iteration runs through the vendored engine for PageRank and SSSP.
- All three transfer engines give identical results on a small graph.
- No `sepgraph::` symbol is visible outside `src/sep_adapter/` and the vendored tree itself.
- All Phase 0–13 tests still pass.

**Time-box: 3–4 days.** This is the first phase with real CUDA dependencies. Expect at least one integration issue with the vendored headers.

---

# Phase 14 Open Questions (to resolve before starting)

1. **Does `framework/framework.cuh` compile as plain C++?** Phase 12's smoke test verified that `framework/common.h` compiles as plain C++. `framework.cuh` is a much larger header and includes `framework/variants/driver.cuh`, which contains kernel-launch syntax. Answer by attempting to include it from a throwaway `.cpp`; if it fails, Phase 14 must introduce a `.cu` file for engine construction.

2. **Which apps does Phase 14 target first?** MASTER_PLAN.md §5 lists PageRank, SSSP, BFS, CC. The vendored SEP-Graph repo has `apps/pr`, `apps/sssp`, `apps/bfs`, `apps/cc` (not copied in Phase 12). Phase 14 can either implement project-owned apps that match SEP-Graph's `TAppImpl` contract, or copy the vendored app headers into `third_party/hytgraph_sep/` as an additive operation. The plan does not specify. **Recommendation: implement project-owned apps under `include/sep_adapter/apps/`** — the paper's algorithms are simple enough and the boundary rule favors project-owned code.

3. **Where does the graph datum get built?** Two possibilities: (a) `sep_graph_datum_adapter.cpp` builds it in one shot; (b) the bridge builds it incrementally per partition. MASTER_PLAN.md §Phase 14 says "Reuse the engine across partitions; only swap the GraphDatum input." This implies (a) builds the datum once, and the bridge reloads it per partition.
