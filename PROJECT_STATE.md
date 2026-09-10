# HyTGraph Reproduction — Project State

## Current Phase

**Phase 11 — Contribution-Driven Scheduling**

## Current Milestone

**M12 — Contribution-Driven Scheduling**

## Current Task

Phase 11 Contribution-Driven Scheduling has been implemented, reviewed, integrated with the CPU/reference algorithm layers, and locally validated. The project is ready to stop at this point, with the next planned phase being Phase 12 — SEP-Graph Foundation.

## Status

**COMPLETE**

The Contribution-Driven Scheduling layer now provides a deterministic reference scheduling abstraction that orders logical work items by supplied contribution/delta priority while preserving the synchronous execution path as the correctness reference.

The implementation explicitly distinguishes:

- PageRank contribution/delta generation
- SSSP contribution generation from tentative-distance improvements
- contribution-prioritized work ordering
- deterministic tie breaking
- synchronous reference ordering
- reordered-work measurement
- zero-contribution measurement
- redundant-work measurement
- explicitly observed stale-work measurement
- reference scheduling-overhead estimation
- validation of non-finite contribution values
- PageRank-to-scheduler integration
- SSSP-to-scheduler integration
- algorithm-specific contribution generation versus generic scheduling

No asynchronous execution speedup, GPU scheduling equivalence, or complete paper-runtime scheduling equivalence is claimed.

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

Partitions expose:

- vertex range
- edge range
- vertex count
- edge count
- target byte size
- edge-data byte size

---

## Phase 4 — ExpTM-Filter

**Status:** COMPLETE

Implemented the reference ExpTM-Filter transfer path and associated transfer metrics.

This is a reference/modeling implementation rather than the paper's complete CUDA execution pipeline.

---

## Phase 5 — Algorithm / Runtime Integration

**Status:** COMPLETE

Integrated the graph/activity/partition/transfer abstractions sufficiently for the current reproduction architecture and validation tests.

---

## Phase 6 — ExpTM-Compaction

**Status:** COMPLETE

Implemented the CPU/reference ExpTM-Compaction path.

Implemented:

- active-edge compaction
- compacted destination/index representation
- transfer-size accounting
- CPU-side compaction accounting
- deterministic reference behavior
- unit tests

The implementation is a reference CPU path and does not claim the paper's full asynchronous GPU/CPU pipeline.

---

## Phase 7 — ImpTM-Zero-Copy

**Status:** COMPLETE

Implemented the ImpTM-Zero-Copy reference/modeling path.

Implemented:

- per-active-vertex zero-copy request counting
- configurable request payload size
- configurable maximum outstanding requests per TLP
- alignment-overhead accounting
- aggregate modeled TLP accounting
- active vertex / active edge metrics
- partition-level metrics
- zero-copy preparation result
- modeled fallback mode
- validation of partition and CSR consistency
- deterministic unit tests

The implementation follows the paper's request-count structure:

    ceil(Do(v) * d1 / m) + am(v)

where:

- `Do(v)` = vertex out-degree
- `d1` = bytes per destination/neighbor entry
- `m` = request payload size
- `am(v)` = alignment overhead indicator

The current implementation keeps:

    memory_requests

and:

    alignment_overhead

as separate metrics.

This is intentional. The Phase 8 cost model combines them according to the paper's zero-copy cost equation.

---

# Phase 7 Files

The following files were added or modified for Phase 7:

    include/transfer/zero_copy_engine.hpp
    src/transfer/zero_copy_engine.cpp
    CMakeLists.txt
    tests/unit_tests.cpp

---

# Phase 7 Validation

The Phase 7 implementation was previously validated by the repository owner.

The current project validation described below supersedes the older Phase 7-only validation state.

---

# Phase 8 — HyTM Cost Model

**Status:** COMPLETE

## Objective

Implemented the HyTM cost model and deterministic transfer-engine selector described in the paper.

The implementation remains scoped to the cost model and selector.

Later scheduling/task-combining phases are implemented separately.

---

## Phase 8 Requirements

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
- unit tests for synthetic partition cases

The selector implements the paper's strict comparison structure.

The cost model operates independently for each logical partition.

---

## Phase 8 Files

The following files were added or modified for Phase 8:

    include/transfer/hytm_cost_model.hpp
    src/transfer/hytm_cost_model.cpp
    CMakeLists.txt
    tests/unit_tests.cpp

---

# Phase 9 — Task Combining

**Status:** COMPLETE

## Objective

Implemented the Task Combining layer that converts the HyTM engine decision for each logical partition into executable tasks.

The implementation follows the reproduction plan's Phase 9 behavior:

- ExpTM-Filter partitions are combined only when consecutive.
- ExpTM-Filter groups contain at most `k` partitions.
- Default Filter combination limit is `k = 4`.
- ExpTM-Compaction partitions selected for the same engine are accumulated into one executable task.
- ImpTM-Zero-Copy partitions selected for the same engine are accumulated into one executable task.
- Final executable tasks are ordered deterministically by their first logical partition.
- Executable tasks receive sequential `task_index` values after final ordering.
- Task-combination metrics report logical partitions, executable tasks, and engine-specific combination counts.

The membership vector:

    partition_indices

is authoritative for executable-task membership.

For consecutive groups, `first_partition_index` / `end_partition_index` also describe the represented partition interval. For globally combined Compaction / Zero-Copy tasks, intervening partitions may belong to another engine, so the membership vector remains authoritative.

---

# Phase 9 Files

The following files were added or modified for Phase 9:

    include/scheduling/task_combiner.hpp
    src/scheduling/task_combiner.cpp
    CMakeLists.txt
    tests/unit_tests.cpp

---

# Phase 9 Tests

The existing namespace-based `tests/unit_tests.cpp` test target was extended with TaskCombiner coverage for:

1. Filter grouping up to the configured `k`.
2. Filter grouping only across consecutive Filter partitions.
3. Compaction partition combination.
4. Zero-Copy partition combination.
5. Reduction from logical partitions to executable tasks.
6. Empty input handling.
7. Sequential executable-task indices.

The tests remain integrated into the existing `unit_tests` executable. No separate Phase 9 test executable was introduced.

---

# Phase 9 Validation Result

The repository owner previously ran:

    cmake --build build -j
    ctest --test-dir build --output-on-failure

Build result:

    PASS

CTest result:

    1/4 Test #1: unit_tests ....................... Passed
    2/4 Test #2: algorithm_tests .................. Passed
    3/4 Test #3: cuda_algorithm_tests ............. Passed
    4/4 Test #4: experiment_runner_smoke .......... Passed

    100% tests passed, 0 tests failed out of 4

The CUDA algorithm tests passed in this validation.

This confirms the Phase 9 build and test state. No additional benchmark or performance claim is made.

---

# Phase 9 Important Implementation Details

## Filter Combination

Default:

    filter_combine_k = 4

A Filter run is grouped only with immediately consecutive Filter partitions.

For example:

    Filter
    Filter
    Compaction
    Filter
    Filter

produces:

    Filter {0, 1}
    Compaction {2}
    Filter {3, 4}

Filter partitions are not combined across another engine selection.

## Compaction Combination

All logical partitions selected for ExpTM-Compaction are represented by one executable Compaction task.

The partition membership vector preserves the original logical partition indices.

## Zero-Copy Combination

All logical partitions selected for ImpTM-Zero-Copy are represented by one executable Zero-Copy task.

The partition membership vector preserves the original logical partition indices.

## Task Ordering

After engine-specific grouping, executable tasks are sorted by:

    first_partition_index

This restores deterministic logical-partition order.

Sequential:

    task_index

values are assigned after this final ordering.

---

# Phase 9 Metrics

`TaskCombinationMetrics` exposes:

    logical_partition_count
    executable_task_count
    filter_partitions_combined
    compaction_partitions_combined
    zero_copy_partitions_combined

The plan also exposes:

    task_count_reduced()
    task_count_reduction()

The metrics describe logical-to-executable task reduction and do not claim measured runtime speedup.

---

# Phase 9 Paper Fidelity

The implementation follows the reproduction plan's paper-aligned task-combination model:

- small logical partitions remain available for fine-grained HyTM engine selection
- Filter combines consecutive selected partitions with a maximum group size of four
- Compaction combines partitions selected for the same engine
- Zero-Copy combines partitions selected for the same engine

The implementation stops at executable-task planning. It does not yet implement the full CUDA task execution, contribution-driven scheduling, neighbor-shifting pipeline, CUDA stream overlap, or complete SEP-Graph integration.

Those later mechanisms remain future work.

---

# Phase 10 — Hub Sorting

**Status:** COMPLETE

## Objective

Implemented the Hub Sorting layer described in the paper and reproduction plan.

The implementation computes a hub importance score for every vertex:

    H(v) = Do(v) * Di(v) / (Do_max * Di_max)

where:

- `Do(v)` = vertex out-degree
- `Di(v)` = vertex in-degree
- `Do_max` = maximum out-degree in the graph
- `Di_max` = maximum in-degree in the graph

The default hub fraction is:

    hub_fraction = 0.08

The selected hubs are placed at the beginning of the vertex ordering.

Non-hub vertices retain their natural vertex-ID order.

Hub sorting is intended as a preparation-time operation rather than an operation performed on every algorithm iteration.

---

## Phase 10 Requirements

Implemented:

- in-degree calculation from CSR adjacency
- out-degree-based hub scoring
- paper-aligned hub importance equation
- configurable hub fraction
- approximate top-fraction hub selection
- deterministic score ordering
- deterministic vertex-ID tie breaking
- hub-first vertex ordering
- natural ordering of non-hubs
- CSR vertex reordering
- CSR destination-ID remapping
- edge-weight preservation
- final CSR validation

The `HubSorter` produces:

    vertex_order
    scores
    hub_count

The ordering uses:

    vertex_order[new_vertex] = old_vertex

The resulting order is then applied to the CSR graph through `CSRGraph::reorder_vertices()`.

---

# Phase 10 Files

The following files were added or modified for Phase 10:

    include/scheduling/hub_sort.hpp
    src/scheduling/hub_sort.cpp
    include/graph/csr_graph.hpp
    src/graph/csr_graph.cpp
    CMakeLists.txt
    tests/unit_tests.cpp

---

# Phase 10 Tests

The existing namespace-based `tests/unit_tests.cpp` test target was extended with Hub Sorting and CSR-reordering coverage for:

1. Hub importance score calculation.
2. Hub selection and hub-first ordering.
3. Deterministic vertex permutation generation.
4. Edgeless graph handling.
5. CSR vertex reordering.
6. CSR destination-ID remapping.
7. CSR edge-weight preservation.
8. Invalid vertex-order rejection.
9. End-to-end HubSorter → CSRGraph reordering integration.

The tests remain integrated into the existing `unit_tests` executable. No separate Phase 10 test executable was introduced.

---

# Phase 10 Validation Result

The repository owner ran:

    ctest --test-dir build --output-on-failure

CTest result:

    1/4 Test #1: unit_tests ....................... Passed
    2/4 Test #2: algorithm_tests .................. Passed
    3/4 Test #3: cuda_algorithm_tests ............. Passed
    4/4 Test #4: experiment_runner_smoke .......... Passed

    100% tests passed, 0 tests failed out of 4

    Total Test time = 2.06 sec

The CUDA algorithm tests passed in this validation.

This confirms the current Phase 10 build/test state supplied by the repository owner.

No additional benchmark or performance claim is made.

---

# Phase 10 Important Implementation Details

## Hub Score

For each vertex:

    H(v) = Do(v) * Di(v) / (Do_max * Di_max)

The implementation first computes the in-degree of every destination vertex from the CSR column indices.

The out-degree is obtained directly from the CSR row offsets.

Vertices are ranked by descending hub score.

Ties are resolved by ascending vertex ID to provide deterministic behavior.

## Hub Selection

The configured fraction is applied to the total vertex count.

The default configuration follows the paper's approximately top-8% hub selection.

The implementation uses a ceiling for positive fractional hub counts, with numerical stabilization around values that are effectively exact integers.

This provides deterministic behavior for small synthetic graphs while preserving the intended approximate top-fraction behavior.

## Hub Ordering

The resulting ordering is:

    hubs in descending score order
    non-hubs in natural vertex-ID order

Only the selected hub prefix is reordered by score.

Non-hub vertices are not score-sorted.

## CSR Reordering

`CSRGraph::reorder_vertices()` interprets the supplied ordering as:

    vertex_order[new_vertex] = old_vertex

The function:

1. validates that the ordering is a complete permutation
2. builds an old-to-new vertex-ID mapping
3. copies each old adjacency list into its new vertex position
4. remaps destination vertex IDs
5. preserves edge weights
6. replaces the CSR arrays
7. validates the resulting CSR graph

The permutation is validated before modifying the graph.

---

# Phase 10 Paper Fidelity

The implementation follows the paper's hub-sorting mechanism:

- vertices with high incoming and outgoing degree receive higher hub scores
- hub importance uses the degree-product score
- approximately the top 8% are selected
- hubs are grouped at the beginning of the CSR ordering
- non-hubs retain natural ordering
- hub sorting is treated as a preparation-time operation

The paper's later hub-driven scheduling behavior is not implemented as part of Phase 10.

The current phase therefore provides the hub ordering foundation required by the later scheduling phase without prematurely introducing unsupported scheduling behavior.

---

# Phase 11 — Contribution-Driven Scheduling

**Status:** COMPLETE

## Objective

Implemented the CPU/reference Contribution-Driven Scheduling layer described by the reproduction plan.

The implementation provides a generic scheduling abstraction that orders already-identified logical work items by supplied contribution/delta values.

The scheduler does not compute algorithm-specific contributions itself. PageRank and SSSP provide algorithm-specific contribution records, while the generic scheduler consumes those values.

The synchronous execution path remains available as the correctness/reference path.

---

## Phase 11 Requirements

Implemented:

- generic contribution-priority representation
- PageRank contribution/delta generation
- SSSP contribution generation
- contribution-driven priority ordering
- deterministic contribution tie breaking
- synchronous reference ordering
- reordered-work measurement
- zero-contribution measurement
- redundant-work measurement
- explicitly observed stale-work measurement
- reference scheduling-overhead estimation
- validation of finite contribution values
- contribution-priority construction helper
- PageRank → scheduler integration
- SSSP → scheduler integration

The scheduler prioritizes larger supplied contribution values first.

For deterministic ties, lower `item_index` values are ordered first.

The implementation does not claim that asynchronous contribution-driven scheduling is automatically faster.

---

## PageRank Contributions

The Phase 11 PageRank layer provides:

    PageRankContribution

with:

    vertex
    contribution

The contribution is derived from the absolute difference between the synchronous next PageRank value and the current rank:

    contribution = |next_rank - current_rank|

The calculation uses the existing CPU PageRank update semantics, including:

- damping factor
- incoming contributions
- dangling mass
- uniform base contribution

This provides a reference delta signal for contribution-driven scheduling without changing the synchronous PageRank algorithm.

---

## SSSP Contributions

The Phase 11 SSSP layer provides:

    SSSPContribution

with:

    vertex
    contribution

The contribution is supplied from tentative-distance changes.

For finite distances:

    contribution = current_distance - candidate_distance

when the candidate represents an improvement greater than the configured tolerance.

Distance increases and unchanged distances produce zero contribution.

For:

    infinity -> finite

the implementation assigns a unit useful-work priority because a finite numeric distance difference cannot be computed from infinity.

This is an engineering scheduling abstraction rather than a claim that the paper specifies this exact SSSP contribution equation.

---

## Contribution Scheduler

The generic scheduler provides:

    ContributionPriority

containing:

    item_index
    contribution

The scheduler produces:

    ContributionSchedulingPlan

containing:

    ordered_item_indices
    metrics

The scheduler:

1. validates contribution values
2. preserves every input work item
3. orders larger contributions first
4. applies deterministic item-index tie breaking
5. records reordered positions
6. records zero/near-zero contributions
7. estimates reference scheduling overhead

The scheduler does not execute the work itself.

---

## Synchronous Reference

The scheduler provides:

    ContributionScheduler::synchronous_reference()

The synchronous reference preserves the supplied work order.

This is intentionally separate from contribution-driven ordering so that correctness/reference execution does not depend on the scheduling heuristic.

No asynchronous execution model is claimed by this phase.

---

## Work Metrics

The Phase 11 metrics expose:

    input_item_count
    scheduled_item_count
    reordered_item_count
    zero_contribution_count
    stale_work_count
    redundant_work_count
    scheduling_overhead

### Reordered Work

`reordered_item_count` reports positions whose scheduled location differs from their original position.

The plan also provides:

    reordered()

which reports whether any item was reordered.

### Zero Contribution

`zero_contribution_count` reports contributions whose absolute magnitude is within the configured zero-contribution epsilon.

Zero contribution is intentionally not classified as stale work.

### Redundant Work

`redundant_work_count` counts duplicate `item_index` requests.

For example:

    {0, 1, 1, 2, 2, 2}

contains:

    3

redundant requests.

### Stale Work

Stale work is not inferred from contribution magnitude.

The execution layer explicitly reports stale observations through:

    ContributionWorkObservation

This keeps stale-work measurement separate from contribution size.

### Scheduling Overhead

The scheduler exposes a deterministic CPU/reference scheduling-overhead estimate.

The current reference model uses:

    log2(N)

where `N` is the number of input work items.

This is a planning-layer estimate only.

It is not a measured GPU execution time, CPU wall-clock benchmark, or claim of runtime overhead equivalence with the paper.

---

# Phase 11 Files

The following files were added or modified for Phase 11:

    include/scheduling/contribution_scheduler.hpp
    src/scheduling/contribution_scheduler.cpp
    include/algorithms/pagerank.hpp
    src/algorithms/pagerank.cpp
    include/algorithms/sssp.hpp
    src/algorithms/sssp.cpp
    CMakeLists.txt
    tests/unit_tests.cpp

---

# Phase 11 Tests

The existing namespace-based `tests/unit_tests.cpp` test target was extended with Contribution-Driven Scheduling coverage for:

1. Contribution ordering by descending priority.
2. Deterministic contribution tie breaking.
3. Zero-contribution epsilon accounting.
4. Reordering metric calculation.
5. Synchronous reference ordering.
6. Redundant-work measurement.
7. Stale-work observation measurement.
8. Contribution-priority construction.
9. Mismatched contribution-input validation.
10. PageRank contribution calculation.
11. PageRank contribution → scheduler integration.
12. SSSP contribution calculation.
13. SSSP contribution → scheduler integration.
14. Reference scheduling-overhead estimation.
15. Scheduling-plan reordered accessor.
16. Non-finite contribution rejection.

The tests remain integrated into the existing `unit_tests` executable.

No separate Phase 11 test executable was introduced.

---

# Phase 11 Validation Result

The repository owner ran:

    cmake --build build -j
    ctest --test-dir build --output-on-failure

Build result:

    PASS

CTest result:

    1/4 Test #1: unit_tests ....................... Passed
    2/4 Test #2: algorithm_tests .................. Passed
    3/4 Test #3: cuda_algorithm_tests ............. Passed
    4/4 Test #4: experiment_runner_smoke .......... Passed

    100% tests passed, 0 tests failed out of 4

The CUDA algorithm tests passed in this validation.

This confirms the current Phase 11 build and test state supplied by the repository owner.

No benchmark or performance equivalence is claimed.

---

# Phase 11 Paper Fidelity

The implementation follows the reproduction plan's contribution-driven scheduling direction:

- work items are prioritized by contribution/delta
- larger contributions receive earlier priority
- synchronous execution remains the correctness reference
- stale/redundant work is explicitly measurable
- scheduling overhead is represented as a reference planning metric

The exact internal scheduler data structures, queue implementation, runtime dependency propagation, and complete asynchronous execution behavior are not sufficiently specified by the available paper material.

Therefore the current implementation is explicitly a **CPU/reference scheduling abstraction**.

The scheduler has not yet been integrated with the SEP-Graph GPU worklist/execution layer.

That integration belongs to the later SEP-Graph phases.

---

# Phase 11 Important Implementation Details

## Generic Contribution Priority

The scheduler consumes:

    ContributionPriority

rather than computing algorithm-specific contributions.

This keeps the scheduling layer independent from PageRank and SSSP implementation details.

## Deterministic Ordering

Contributions are ordered by descending value.

For equal contributions, the scheduler uses ascending `item_index` when deterministic tie breaking is enabled.

This ensures repeatable scheduling plans for identical inputs.

## PageRank Reference Delta

PageRank contributions are computed from one synchronous update using the existing CPU PageRank semantics.

The algorithm itself remains synchronous.

The contribution output is therefore a scheduling signal rather than an asynchronous execution mechanism.

## SSSP Reference Contribution

SSSP contributions are based on externally supplied tentative-distance changes.

The scheduler does not fabricate an unsupported paper-specific SSSP equation.

## Stale Work

Stale work is represented through explicit execution observations.

The scheduler does not assume:

    zero contribution == stale work

This distinction is intentional.

## Redundant Work

Redundant work is measured from duplicate logical work-item identifiers.

This provides an objective reference metric without requiring assumptions about downstream execution state.

## Scheduling Overhead

The current scheduling overhead is a deterministic reference estimate:

    log2(N)

It is not a measured runtime quantity.

---

# Known Issues / Engineering Approximations

The following are known and intentionally documented.

## 1. Exact Original Partition Boundaries

The paper does not provide enough information to reproduce every original runtime partition boundary exactly.

The project therefore uses deterministic logical partitions.

## 2. Logical Partitioning

Logical partitions currently serve as graph-analysis/reference abstractions.

They are not yet connected to a complete runtime scheduler.

## 3. ExpTM-Filter

The current implementation is a reference transfer model rather than the complete CUDA transfer mechanism.

## 4. ExpTM-Compaction

The current implementation is CPU/reference compaction.

It does not claim to reproduce the paper's complete asynchronous execution pipeline.

## 5. Subway

The exact internal Subway implementation and scheduling behavior are not fully specified by the available sources.

No unsupported implementation details are being invented.

## 6. Physical Transfer Accounting

Current transfer sizes are logical/reference byte counts.

They are not claimed to represent every physical PCIe transaction or runtime metadata transfer.

## 7. CUDA Execution

The transfer-engine, task-combination, hub-sorting, and contribution-scheduling layers remain reference/modeling or preparation components.

Passing CUDA algorithm tests does not mean the complete HyTGraph CUDA runtime has been reproduced.

## 8. Zero-Copy Mapping

Phase 7 does not perform actual CUDA pinned host allocation, host registration, or mapped-memory pointer acquisition.

The behavior is explicitly modeled.

## 9. Zero-Copy Alignment

The zero-copy alignment calculation uses a logical CSR byte-offset proxy.

It is not a physical host-memory address calculation.

## 10. Compaction Throughput

A reproducible paper-specific CPU compaction throughput measurement is not currently available from the project sources.

Phase 8 exposes throughput as a configurable model parameter rather than inventing a paper-specific measured value.

## 11. Task Combination

The current TaskCombiner is an executable-task planning layer.

It does not yet execute grouped tasks, overlap transfers and computation, or implement the paper's complete scheduling pipeline.

## 12. Task Ordering for Globally Combined Engines

Compaction and Zero-Copy groups may contain non-consecutive logical partition indices because they are accumulated by engine selection.

`partition_indices` is therefore authoritative for those task memberships.

## 13. CMake CUDA Architecture Default

The build was adjusted so that when CUDA is enabled and no explicit architecture is supplied, CMake uses:

    CMAKE_CUDA_ARCHITECTURES=native

This is a build-configuration convenience to avoid guessing a GPU architecture. It is not a HyTGraph algorithmic behavior and does not claim paper fidelity.

## 14. No Benchmark Claims

No performance or benchmark equivalence to the original HyTGraph implementation is currently claimed.

## 15. Hub Count for Small Graphs

The paper specifies approximately the top 8% but does not specify exact rounding behavior for very small graphs.

The implementation uses deterministic ceiling behavior for positive fractional counts, with numerical stabilization around exact integer values.

This is an engineering approximation for small synthetic graphs.

## 16. Zero-Degree Graphs

For graphs where the maximum in-degree or maximum out-degree is zero, the hub-score denominator is undefined.

The implementation assigns zero scores in this case rather than performing division by zero.

This is an engineering edge-case decision.

## 17. Vertex Renumbering

`CSRGraph::reorder_vertices()` changes the internal vertex numbering according to the supplied ordering and remaps destination IDs accordingly.

Hub sorting is therefore intended to run during preparation rather than repeatedly after algorithm state has been established.

Callers that maintain external vertex-ID state must account for the resulting renumbering.

## 18. Contribution-Driven Scheduling

The Phase 11 scheduler is a CPU/reference planning abstraction.

It does not yet:

- execute asynchronous work
- maintain a GPU work queue
- integrate with SEP-Graph worklists
- coordinate CUDA streams
- perform neighbor shifting
- overlap CPU compaction with GPU execution
- measure actual scheduler wall-clock overhead
- claim the paper's complete contribution-driven runtime behavior

These mechanisms remain future work in the later SEP-Graph and HyTGraph integration phases.

## 19. SSSP Contribution Definition

The available paper material does not specify a complete formal SSSP contribution equation.

The current implementation therefore uses tentative-distance improvement supplied by the execution layer.

This is explicitly an engineering approximation and not presented as a recovered paper formula.

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

The repository's GitHub state may not contain the user's unpushed local changes. The user's local working tree is authoritative for newly implemented phases until those changes are committed/pushed by the repository owner.

---

# Current Validation Status

Last confirmed project validation:

    cmake --build build -j
    ctest --test-dir build --output-on-failure
    PASS

CTest result:

    100% tests passed
    0 tests failed
    4 tests passed

Individual tests:

    unit_tests ....................... Passed
    algorithm_tests .................. Passed
    cuda_algorithm_tests ............. Passed
    experiment_runner_smoke .......... Passed

CUDA algorithm tests:

    PASSED

The current Phase 11 validation confirms the Contribution-Driven Scheduling layer, PageRank contribution generation, SSSP contribution generation, existing algorithm tests, CUDA algorithm tests, and experiment-runner smoke test targets are passing.

No benchmark or performance equivalence is claimed.

---

# Current Milestone

    M12 — Contribution-Driven Scheduling

Status:

    COMPLETE

---

# Next Milestone

    M13 — SEP-Graph Foundation

Status:

    READY

---

# NEXT TASK

**Next task:** Phase 12 — SEP-Graph Foundation.

The project is intentionally stopped here for this handoff.

Before implementation resumes, inspect the relevant Phase 12 section of `MASTER_PLAN.md`, the original paper mechanism, and the current repository state. Then provide only the first required file and wait for local validation.
