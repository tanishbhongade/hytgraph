# HyTGraph Reproduction — Project State

## Current Phase

**Phase 12 — SEP-Graph Foundation**

## Current Milestone

**M13 — SEP-Graph Foundation**

## Current Task

Phase 12 SEP-Graph Foundation has been implemented as a project-local execution abstraction layer.

The Phase 12 foundation establishes:

- SEP execution variants
- execution configuration
- execution context
- execution requirements
- execution plans
- execution selection
- execution result representation
- execution frontiers
- frontier adaptation
- execution-driver interfaces
- deferred execution drivers
- execution factory
- SEP application contract
- SEP application adapters
- SEP application traits
- compile-time foundation validation

The implementation intentionally stops before concrete CUDA execution.

The current Phase 12 layer provides the semantic boundary required for later SEP-Graph execution integration without claiming that the complete SEP-Graph runtime has already been reproduced.

---

# Status

**COMPLETE**

Phase 12 establishes the SEP-Graph execution foundation required for the later CUDA/device integration phase.

The implementation is currently a structural/reference execution layer.

It does **not** yet provide:

- concrete CUDA SEP kernels
- GPU worklist execution
- CUDA stream coordination
- actual device-side frontier execution
- complete asynchronous execution
- complete topology-driven GPU execution
- complete data-driven GPU execution
- paper-runtime performance equivalence

Those mechanisms remain future work.

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

# Phase 12 — SEP-Graph Foundation

**Status:** COMPLETE

## Objective

Establish a project-local SEP-Graph execution foundation that captures the semantic execution variants and application/execution boundaries required for later CUDA integration.

The implementation intentionally avoids introducing dependencies on the project's concrete CUDA graph/runtime implementation at this stage.

The Phase 12 foundation is therefore an architectural and structural execution layer rather than a complete GPU execution implementation.

---

## Phase 12 Requirements

Implemented:

- SEP execution variant representation
- SYNC/ASYNC execution modes
- PUSH/PULL message-passing modes
- DATA_DRIVEN/TOPOLOGY_DRIVEN scheduling modes
- canonical SEP variant names
- variant parsing
- variant validation
- execution configuration
- execution context
- graph/context structural requirements
- frontier requirements
- execution plans
- execution-selection abstraction
- execution results
- execution-driver base interface
- deferred execution driver
- execution factory
- execution frontier abstraction
- frontier adapter
- SEP application contract
- application adapter
- application type traits
- application compatibility detection
- compile-time foundation validation

The eight execution combinations represented by the Phase 12 variant model are:

    SYNC_PUSH_DD
    SYNC_PULL_DD
    SYNC_PUSH_TD
    SYNC_PULL_TD
    ASYNC_PUSH_DD
    ASYNC_PULL_DD
    ASYNC_PUSH_TD
    ASYNC_PULL_TD

---

# Phase 12 Execution Variant

The execution variant is represented as the Cartesian product of:

    ExecutionMode
    MessagePassing
    SchedulingMode

with:

    ExecutionMode:
        SYNC
        ASYNC

    MessagePassing:
        PUSH
        PULL

    SchedulingMode:
        DATA_DRIVEN
        TOPOLOGY_DRIVEN

All eight combinations are structurally valid at the Phase 12 abstraction level.

The variant also provides canonical string conversion/parsing so execution-selection code does not need to depend on enum implementation details.

---

# Phase 12 Execution Requirements

The structural execution requirements are intentionally limited.

At this phase:

- a graph must be present in the execution context;
- DATA_DRIVEN variants require a frontier;
- TOPOLOGY_DRIVEN variants do not require a frontier.

The requirements layer does not validate:

- CUDA availability
- GPU memory
- graph correctness beyond the execution-context contract
- partition correctness
- algorithm convergence
- device transfer state
- kernel availability
- runtime performance

Those checks belong to later execution/integration layers.

---

# Phase 12 Execution Context

The execution context provides the runtime-independent state required by execution selection and execution planning.

The context establishes whether the execution environment contains the structural resources required by a selected variant.

In particular, the context can determine whether:

- a graph is present;
- a frontier is present when required;
- the selected execution variant is structurally satisfiable.

The context remains independent from concrete CUDA/device state.

---

# Phase 12 Execution Plan

The execution plan provides a deterministic representation of the selected execution configuration.

It separates:

- execution selection
- execution requirements
- execution configuration
- later execution

This allows the execution layer to determine what should execute without requiring the current phase to actually launch kernels.

---

# Phase 12 Execution Selection

Execution selection provides a deterministic mapping from an explicitly supplied execution configuration to the corresponding execution variant/plan.

No automatic performance heuristic or runtime switching is introduced at this phase.

The selection layer therefore does not claim to reproduce any undocumented SEP runtime scheduling heuristic.

---

# Phase 12 Execution Result

The execution result provides a common representation for execution-step outcomes.

The Phase 12 foundation includes result states for cases such as:

- successful execution
- initialization requirements
- invalid configuration
- deferred/unimplemented execution

The result abstraction allows later CUDA-backed execution drivers to report execution state without changing the higher-level execution interfaces.

---

# Phase 12 Execution Drivers

The execution-driver hierarchy establishes the boundary between execution planning and actual graph execution.

The base driver provides the common interface for:

- initialization
- execution-context binding
- execution-step execution
- selected execution-variant reporting

Phase 12 supplies a deferred execution driver.

The deferred driver deliberately does not execute graph work.

Instead, an execution step reports the appropriate non-concrete execution state because the actual CUDA implementation is deferred to the later SEP execution phase.

---

# Phase 12 Execution Factory

The execution factory creates an execution driver for an explicitly selected SEP variant.

The factory:

- validates the selected variant;
- accepts all eight structurally valid variants;
- creates a deferred Phase 12 driver;
- supports creation from `SEPExecutionVariant`;
- supports creation from `SEPExecutionConfig`;
- supports creation from canonical variant strings.

The factory does not perform heuristic runtime selection.

---

# Phase 12 Frontier

The SEP frontier abstraction represents the logical active work set required by data-driven execution.

The frontier interface exposes operations for:

- clearing the frontier
- pushing a node
- popping a node
- checking size
- checking emptiness

The Phase 12 frontier remains an execution abstraction.

It does not claim to reproduce the complete SEP-Graph GPU worklist implementation.

---

# Phase 12 Frontier Adapter

The frontier adapter provides a boundary between the SEP execution layer and a future/project-local frontier implementation.

The adapter keeps concrete worklist implementation details outside the execution contract.

Actual GPU/device frontier management remains future work.

---

# Phase 12 SEP Application Contract

`SEPApplication` establishes the application-facing semantic contract for SEP execution.

The interface captures:

- execution-variant configuration
- initial vertex values
- initial buffers
- buffer identity
- value/buffer combination
- message accumulation
- weighted message accumulation
- activity testing
- post-computation hooks
- priority testing
- edge-weight requirements
- activity-predicate support

The contract is intentionally independent of:

- `CSRGraph`
- `ActivityTracker`
- `LogicalPartition`
- concrete Task classes
- concrete worklists
- CUDA device structures

This allows PageRank and SSSP to provide SEP semantics without forcing them into a new concrete runtime hierarchy.

---

# Phase 12 Application Adapter

`SEPApplicationAdapter` provides a non-owning adapter around an existing application implementation.

The adapter:

- does not own the underlying application;
- does not copy the underlying application;
- forwards SEP application operations;
- forwards execution-variant configuration;
- exposes the underlying application when required;
- avoids imposing a new inheritance hierarchy on existing PageRank/SSSP implementations.

The adapter is intended to bridge existing project algorithms into the SEP execution boundary.

---

# Phase 12 Application Traits

`SEPApplicationTraits` provides compile-time execution-level information about applications.

The traits expose:

- application value type
- buffer type
- weight type
- node ID type
- edge-weight requirements
- activity-predicate support
- SEP application compatibility detection

Additional trait definitions describe PageRank-like and SSSP-like execution requirements.

SSSP is represented as requiring edge weights.

PageRank is represented as not requiring explicit per-edge weights for the SEP accumulation abstraction.

These traits describe execution requirements and do not implement algorithm execution.

---

# Phase 12 Files

The following SEP foundation headers were added for Phase 12:

    include/sep/sep_application.hpp
    include/sep/sep_application_adapter.hpp
    include/sep/sep_application_traits.hpp
    include/sep/sep_execution_config.hpp
    include/sep/sep_execution_context.hpp
    include/sep/sep_execution_driver.hpp
    include/sep/sep_execution_driver_base.hpp
    include/sep/sep_execution_factory.hpp
    include/sep/sep_execution_plan.hpp
    include/sep/sep_execution_requirements.hpp
    include/sep/sep_execution_result.hpp
    include/sep/sep_execution_selection.hpp
    include/sep/sep_execution_variant.hpp
    include/sep/sep_execution_variant_registry.hpp
    include/sep/sep_frontier.hpp
    include/sep/sep_frontier_adapter.hpp

The existing consolidated test target was also extended:

    tests/unit_tests.cpp

Phase-specific standalone tests were used during development and validation of the individual SEP foundation components.

The project's final test organization remains centered around the existing `unit_tests` target.

---

# Phase 12 Tests

Phase 12 testing covered the SEP foundation components, including:

1. SEP execution variant construction.
2. SEP execution variant equality/accessors.
3. SEP execution variant string representation.
4. SEP execution variant parsing.
5. SEP execution variant validation.
6. SEP execution configuration defaults.
7. SEP execution configuration mutation.
8. SEP execution context graph presence.
9. SEP execution context frontier requirements.
10. Execution requirement validation.
11. Execution plan construction.
12. Execution plan variant/configuration preservation.
13. Execution selection behavior.
14. Execution result state representation.
15. Execution frontier behavior.
16. Frontier clearing.
17. Frontier push/pop behavior.
18. Frontier reuse.
19. Deferred execution factory behavior.
20. Execution driver interface behavior.
21. SEP application polymorphism.
22. SEP application adapter interface compatibility.
23. SEP application trait type detection.
24. SEP application compatibility detection.
25. Compile-time SEP foundation header compatibility.

The Phase 12 tests were consolidated into the existing project unit-test infrastructure rather than introducing a permanent collection of independent test executables.

---

# Phase 12 Validation

The individual Phase 12 foundation tests were compiled and executed during implementation.

The validated test components included:

    sep_execution_variant_test
    sep_execution_config_test
    sep_execution_context_test
    sep_execution_requirements_test
    sep_execution_plan_test
    sep_execution_selection_test
    sep_frontier_test
    sep_execution_factory_test
    sep_execution_driver_test
    sep_application_test
    sep_application_adapter_test
    sep_application_traits_test
    sep_foundation_compile_test

The owner subsequently consolidated the relevant Phase 12 test coverage into the project's existing `tests/unit_tests.cpp`.

The final repository-wide validation should be considered authoritative once the consolidated `unit_tests` target has been rebuilt and executed after the Phase 12 merge.

No benchmark or performance-equivalence claim is made.

---

# Phase 12 Paper Fidelity

The Phase 12 implementation follows the SEP-Graph execution model at the semantic boundary level.

It explicitly represents:

- synchronous execution
- asynchronous execution
- push message passing
- pull message passing
- data-driven scheduling
- topology-driven scheduling
- frontier-driven execution requirements
- application-defined update semantics
- application-defined activity semantics
- application-defined priority semantics

The implementation does not claim to reproduce undocumented internal SEP-Graph implementation details.

In particular, Phase 12 does not claim to reproduce:

- exact CUDA kernel implementations
- exact GPU worklist structures
- exact memory layouts
- exact CUDA scheduling behavior
- exact stream behavior
- exact device-side synchronization
- exact performance characteristics

Those mechanisms belong to later implementation phases.

---

# Phase 12 Important Implementation Details

## Eight SEP Variants

The foundation models eight execution combinations:

    SYNC × PUSH × DATA_DRIVEN
    SYNC × PULL × DATA_DRIVEN
    SYNC × PUSH × TOPOLOGY_DRIVEN
    SYNC × PULL × TOPOLOGY_DRIVEN

    ASYNC × PUSH × DATA_DRIVEN
    ASYNC × PULL × DATA_DRIVEN
    ASYNC × PUSH × TOPOLOGY_DRIVEN
    ASYNC × PULL × TOPOLOGY_DRIVEN

These combinations are represented explicitly rather than encoded through implicit boolean flags.

---

## Data-Driven Execution

Data-driven variants require a frontier.

The frontier identifies candidate active work.

The Phase 12 abstraction does not require the execution driver to scan the entire graph in order to identify work.

The actual GPU worklist implementation remains future work.

---

## Topology-Driven Execution

Topology-driven variants do not structurally require a frontier.

The application can provide an activity predicate through the SEP application contract.

This establishes the semantic boundary for topology-driven execution without implementing the eventual GPU traversal mechanism.

---

## Weighted Execution

The application contract supports both unweighted and weighted accumulation.

PageRank can use the unweighted accumulation operation.

SSSP can use the weighted accumulation operation.

The execution foundation therefore does not need to inspect concrete application types to determine whether weighted edges are semantically required.

---

## Deferred Execution

The Phase 12 factory returns deferred drivers.

This is intentional.

The foundation establishes the execution architecture without falsely implying that a selected SEP variant is already executable on the GPU.

Concrete CUDA execution belongs to the next implementation stage.

---

# Known Issues / Engineering Approximations

The following remain known and intentional.

## 1. Exact Original Partition Boundaries

The paper does not provide enough information to reproduce every original runtime partition boundary exactly.

The project therefore uses deterministic logical partitions.

---

## 2. Logical Partitioning

Logical partitions remain graph-analysis/reference abstractions rather than a complete reproduction of the paper's runtime partition scheduler.

---

## 3. ExpTM-Filter

The current implementation remains a reference transfer model rather than the complete CUDA transfer mechanism.

---

## 4. ExpTM-Compaction

The current implementation remains CPU/reference compaction.

It does not claim to reproduce the complete asynchronous execution pipeline.

---

## 5. Subway

The exact internal Subway implementation and scheduling behavior are not fully specified by the available sources.

No unsupported implementation details are being invented.

---

## 6. Physical Transfer Accounting

Current transfer sizes are logical/reference byte counts.

They are not claimed to represent every physical PCIe transaction or runtime metadata transfer.

---

## 7. CUDA Execution

The transfer-engine, task-combination, hub-sorting, contribution-scheduling, and SEP foundation layers remain reference/modeling, preparation, or architectural components until concrete CUDA execution is implemented.

Passing CUDA algorithm tests does not mean the complete HyTGraph CUDA runtime has been reproduced.

---

## 8. Zero-Copy Mapping

Phase 7 does not perform actual CUDA pinned host allocation, host registration, or mapped-memory pointer acquisition.

The behavior is explicitly modeled.

---

## 9. Zero-Copy Alignment

The zero-copy alignment calculation uses a logical CSR byte-offset proxy.

It is not a physical host-memory address calculation.

---

## 10. Compaction Throughput

A reproducible paper-specific CPU compaction throughput measurement is not currently available from the project sources.

Phase 8 exposes throughput as a configurable model parameter rather than inventing a paper-specific measured value.

---

## 11. Task Combination

The current TaskCombiner is an executable-task planning layer.

It does not yet execute grouped tasks, overlap transfers and computation, or implement the paper's complete scheduling pipeline.

---

## 12. Task Ordering for Globally Combined Engines

Compaction and Zero-Copy groups may contain non-consecutive logical partition indices because they are accumulated by engine selection.

`partition_indices` is therefore authoritative for those task memberships.

---

## 13. CMake CUDA Architecture Default

When CUDA is enabled and no explicit architecture is supplied, CMake uses:

    CMAKE_CUDA_ARCHITECTURES=native

This is a build-configuration convenience and is not a HyTGraph algorithmic behavior.

---

## 14. No Benchmark Claims

No performance or benchmark equivalence to the original HyTGraph or SEP-Graph implementation is currently claimed.

---

## 15. Hub Count for Small Graphs

The paper specifies approximately the top 8% but does not specify exact rounding behavior for very small graphs.

The implementation uses deterministic ceiling behavior for positive fractional counts.

This is an engineering approximation for small synthetic graphs.

---

## 16. Zero-Degree Graphs

For graphs where maximum in-degree or maximum out-degree is zero, the hub-score denominator is undefined.

The implementation assigns zero scores rather than performing division by zero.

---

## 17. Vertex Renumbering

`CSRGraph::reorder_vertices()` changes internal vertex numbering according to the supplied ordering and remaps destination IDs accordingly.

Callers maintaining external vertex-ID state must account for the resulting renumbering.

---

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

---

## 19. SSSP Contribution Definition

The available paper material does not specify a complete formal SSSP contribution equation.

The current implementation therefore uses tentative-distance improvement supplied by the execution layer.

This is explicitly an engineering approximation.

---

## 20. SEP CUDA Execution

Phase 12 does not yet provide concrete CUDA-backed implementations for the eight SEP execution variants.

The current factory intentionally returns deferred execution drivers.

The next SEP implementation phase must provide the concrete mapping between:

    SEPExecutionVariant

and:

    CUDA/device execution

including the appropriate:

- graph traversal;
- push/pull behavior;
- data-driven/topology-driven behavior;
- synchronous/asynchronous behavior;
- frontier/worklist management;
- application operations;
- device-side state.

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

## Phase 12 Component Validation

The individual Phase 12 foundation components were compiled and executed successfully during development after resolving the corresponding interface/test mismatches.

The consolidated project test file now contains the Phase 12 test coverage.

The remaining authoritative validation step is the repository-level build/test after the consolidated `unit_tests.cpp` changes have been incorporated.

No performance claim is made.

---

# Current Milestone

    M13 — SEP-Graph Foundation

Status:

    COMPLETE

---

# Next Milestone

    M14 — SEP-Graph CUDA Execution

Status:

    READY

---

# NEXT TASK

**Next task:** Phase 13 — SEP-Graph CUDA Execution.

The next phase should begin by inspecting:

- the Phase 13 section of `MASTER_PLAN.md`;
- the original SEP-Graph paper;
- the current Phase 12 execution interfaces;
- the current CUDA algorithm/runtime architecture.

The implementation should then introduce the first concrete CUDA/device execution component while preserving the Phase 12 semantic boundary.

The first Phase 13 implementation must not prematurely replace the Phase 12 abstractions or introduce unsupported runtime behavior.
