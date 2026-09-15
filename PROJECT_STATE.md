# HyTGraph Reproduction — Project State

## Current Phase

**Phase 13 — Adapter layer (no data movement)** _(ready to start)_

## Current Milestone

**M13 — SEP execution driver adapter** _(READY)_

## Current Task

Phase 12 is complete. Phase 13 will introduce the `sep_adapter` module as the single boundary between project code and the vendored SEP-Graph + Groute code. Phase 13 implements a thin wrapper around the vendored engine; it performs no data movement.

---

# Status

Phases 0–12 are complete and validated in the user's local working tree.

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

# Planned Future Phases (not yet started)

| Phase    | Name                             | Status      |
| -------- | -------------------------------- | ----------- |
| Phase 13 | Adapter layer (no data movement) | READY       |
| Phase 14 | Data-movement bridge             | NOT STARTED |
| Phase 15 | Task combining bridge            | NOT STARTED |
| Phase 16 | Contribution scheduling bridge   | NOT STARTED |
| Phase 17 | VCGC read path                   | NOT STARTED |
| Phase 18 | VCGC refresh                     | NOT STARTED |
| Phase 19 | Multi-stream runtime             | NOT STARTED |
| Phase 20 | Full HyTGraph integration        | NOT STARTED |
| Phase 21 | Evaluation                       | NOT STARTED |

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

## Superseded Work

The earlier draft Phase 12 ("SEP-Graph Foundation" — project-local abstraction layer) is **superseded and discarded**. Its files (`include/sep/sep_*.hpp`) are no longer part of the architecture. If they exist in the local tree, they should be removed.

---

# Current Milestone

    M13 — SEP execution driver adapter

Status:

    READY

---

# Next Milestone

    M14 — HyTGraph ↔ SEP data-movement bridge

Status:

    PENDING

---

# NEXT TASK

**Next task:** Phase 13 — Adapter layer (no data movement).

Phase 13 introduces the `sep_adapter` module as the single boundary between project code and vendored code. It implements a thin wrapper around the vendored engine. No data movement, no kernel launches, no partition bridging.

Phase 13 deliverables:

1. `include/sep_adapter/sep_variant.hpp` — project-owned `SEPVariant` enum.
2. `include/sep_adapter/sep_execution_result.hpp` — `SEPExecutionResult` with `State { OK, NEED_INIT, INVALID_CONFIG, DEFERRED, FAILED }`.
3. `include/sep_adapter/sep_execution_driver.hpp` — abstract `SEPExecutionDriver` base class.
4. `include/sep_adapter/sep_variant_mapper.hpp` + `src/sep_adapter/sep_variant_mapper.cpp` — bijective mapping between project enum and `sepgraph::common::AlgoVariant`. The **only** file that may include `algo_variants.cuh`.
5. `include/sep_adapter/sep_engine_adapter.hpp` + `src/sep_adapter/sep_engine_adapter.cpp` — templated `SEPEngineAdapter<TApp>` with pimpl.
6. `include/sep_adapter/sep_engine_factory.hpp` + `src/sep_adapter/sep_engine_factory.cpp` — factory producing a driver per `(algorithm, variant)` pair.
7. `include/sep_adapter/null_sep_driver.hpp` — null driver returning `DEFERRED`, allows testing the abstraction without vendored code.

Exit criteria for Phase 13:

- Toy app (4 vertices) executes one SEP step through the adapter.
- Adapter `.hpp` files contain no vendored symbols.
- All Phase 0–12 tests still pass.

Do **not** add data movement in Phase 13. Do **not** modify the vendored tree. Do **not** edit any file outside `include/sep_adapter/` and `src/sep_adapter/` except the root `CMakeLists.txt` (to wire the new sources into an existing or new target).

**Time-box: 2 days.**
