# HyTGraph Reproduction — Project State

## Current Phase

Phase 6 — ExpTM-Compaction

## Current Milestone

M7 — ExpTM-Compaction

## Current Task

Implement ExpTM-Compaction after completing the Phase 5 ExpTM-Filter baseline.

---

# Completed Phases

## Phase 0 — Project Infrastructure

Implemented:

- CMake project structure
- Runtime library
- Test infrastructure
- CUDA build target
- Configuration infrastructure
- Logging/result infrastructure

---

## Phase 1 — CSR Graph Infrastructure

Implemented:

- CSR graph representation
- CSR validation
- Graph loading
- Graph-related unit tests

---

## Phase 2 — Reference Algorithms

Implemented:

- CPU PageRank
- CPU SSSP
- GPU baseline PageRank
- GPU baseline SSSP
- CPU/GPU correctness tests

The GPU implementation established in Phase 2 is the baseline GPU infrastructure.

HyTGraph-specific GPU transfer/execution mechanisms are introduced only in the phases where they are required.

---

## Phase 3 — Activity Tracking

Implemented:

- Reusable CPU ActivityTracker
- Active-vertex tracking
- Active-edge counting
- Partition statistics
- Integration with SSSP

Activity tracking remains independent of logical partitioning.

---

## Phase 4 — Logical Partitioning

Implemented:

- LogicalPartition
- LogicalPartitioner
- Configurable logical partition target size
- Default 32 MiB logical partition target
- CSR-row-aligned partition boundaries
- Partition vertex ranges
- Partition edge ranges
- Partition edge-byte accounting
- Partition validation
- Unit tests for partition correctness

### Partitioning Behavior

Logical partitions are represented as metadata over the existing CSR graph.

A partition contains:

- `[vertex_begin, vertex_end)`
- `[edge_begin, edge_end)`
- configured target size

The partitioner walks CSR vertex rows and accumulates the corresponding edge storage.

A partition boundary is created before a vertex when adding that vertex would cause the current non-empty partition to exceed the configured target size.

CSR rows are never split.

The configured partition size is therefore a target rather than an exact size.

A vertex with an adjacency list larger than the configured target may produce an oversized partition.

### Phase 4 Correctness Invariants

- First partition begins at vertex 0.
- Last partition ends at `num_vertices`.
- Partitions have contiguous vertex ranges.
- Partitions have contiguous edge ranges.
- No vertex is assigned to more than one partition.
- No edge range is skipped between partitions.
- Sum of partition edge counts equals graph edge count.
- CSR row boundaries are preserved.
- Empty graphs produce no partitions.
- Zero-degree vertices are supported.
- Oversized adjacency lists remain intact.
- Zero partition size is rejected.

### Phase 4 Files

Added:

- `include/graph/partition.hpp`
- `src/graph/partition.cpp`

Modified:

- `CMakeLists.txt`
- `tests/unit_tests.cpp`

---

# Phase 5 — ExpTM-Filter

## Status

**COMPLETE — reference/filter-planning baseline**

Phase 5 has been completed at the reference/filter-planning level.

The implementation provides the required logical-partition filtering behavior and transfer-volume accounting.

Actual HyTGraph-specific CUDA host-to-device transfer execution is intentionally not part of this completed reference layer.

---

## Phase 5 Implementation

Implemented:

- Reference CPU ExpTM-Filter
- Integration with existing LogicalPartition
- Integration with existing ActivityTracker statistics
- Active-partition selection
- Inactive-partition skipping
- Whole-logical-partition transfer semantics
- Disabled-filter/reference path
- Per-partition transfer-selection state
- Transferred partition count
- Transferred edge count
- Transferred byte count
- ActivityTracker/graph vertex-count validation
- Partition range/coverage validation

### ExpTM-Filter Semantics

For a partition containing active edges:

    active_edges > 0
            ↓
    partition selected
            ↓
    complete logical partition transferred

Active edges are not compacted by ExpTM-Filter.

For example:

    active_edges = 1
    partition_edges = 100

results in:

    transferred_edges = 100

For a partition with no active edges:

    active_edges = 0
            ↓
    partition skipped

The disabled-filter/reference path selects all logical partitions.

---

## Phase 5 Measurements

The filter plan records:

- active partition count
- transferred partition count
- transferred edge count
- transferred byte count

Transferred-byte accounting is based on the logical partition edge payload exposed by the partition abstraction.

These values are logical/reference transfer measurements.

They are not measurements of physical PCIe/H2D traffic.

No artificial GPU timing was introduced into the filter-planning layer.

---

## Phase 5 Tests

Tests cover:

- Active/inactive partition filtering
- Whole-partition transfer semantics
- Disabled-filter/reference behavior
- Activity/graph size mismatch
- Transferred-edge accounting
- Transferred-byte accounting

The project owner reported that the complete local CTest suite passed after the Phase 5 changes.

Reported result:

    100% tests passed, 0 tests failed out of 4

Tests reported as passing:

- `unit_tests`
- `algorithm_tests`
- `cuda_algorithm_tests`
- `experiment_runner_smoke`

These tests were executed by the project owner, not by the assistant.

---

## Phase 5 Files

Added:

- `include/transfer/filter_engine.hpp`
- `src/transfer/filter_engine.cpp`

Modified:

- `CMakeLists.txt`
- `tests/unit_tests.cpp`

---

# Paper Features

- [x] CSR
- [x] Logical partitioning
- [ ] SEP-Graph / equivalent GPU kernel
- [ ] Neighbor shifting
- [x] ExpTM-Filter
- [ ] ExpTM-Compaction
- [ ] ImpTM-Zero-Copy
- [ ] HyTM
- [ ] Task Combining
- [ ] Hub Sorting
- [ ] Contribution-Driven Scheduling
- [ ] VCGC
- [ ] Cache Refresh
- [ ] Multi-stream execution

---

# Algorithms

- [x] PageRank
- [x] SSSP
- [ ] BFS
- [ ] Connected Components

---

# Datasets

No benchmark dataset has been integrated yet.

No benchmark measurements have been collected.

---

# Paper Fidelity

## ExpTM-Filter

The Phase 5 reference implementation follows the documented whole-partition filtering behavior:

- Logical partitions are the filtering granularity.
- Partitions containing active edges are selected.
- Partitions containing no active edges are skipped.
- Selected partitions are represented as complete logical-partition transfers.
- Active-edge compaction is not performed by ExpTM-Filter.

The implementation does not claim to reproduce undocumented internal details of the original HyTGraph runtime.

## Logical Partitioning

The reproduction uses logical partitions as a fine-grained graph-analysis abstraction.

The partition target is configured with a 32 MiB default.

Partition boundaries preserve CSR adjacency-list boundaries.

---

# Engineering Approximations

## Logical Partitioning

The supplied paper material does not provide sufficient implementation detail to reconstruct the exact original partition-boundary construction algorithm.

The reproduction therefore uses:

- CSR vertex-row boundaries.
- Target-size balancing based on CSR destination storage.
- No splitting of individual CSR adjacency rows.
- Metadata-only logical partitions referencing the existing CSR graph.

These are engineering approximations and should not be interpreted as confirmed implementation details of the original HyTGraph system.

The 32 MB terminology is represented as 32 MiB / 33,554,432 bytes in the implementation.

## ExpTM-Filter

The current implementation separates:

1. deciding which logical partitions should be transferred, and
2. physically executing those transfers on the GPU.

The Phase 5 implementation completes the first part.

`transferred_bytes` represents the logical partition payload exposed by the partition abstraction.

It should not be interpreted as a measurement of physical PCIe/H2D traffic.

Actual CUDA transfer execution is deferred to the phase where the transfer mechanism is explicitly integrated.

---

# Dependencies

Phase 5 uses:

- CSRGraph
- LogicalPartition
- LogicalPartitioner
- ActivityTracker

Phase 5 does not yet integrate:

- SEP-Graph
- Subway
- CUB
- CUDA streams
- Neighbor shifting
- ExpTM-Compaction
- ImpTM-Zero-Copy
- VCGC
- HyTM

The existing GPU baseline infrastructure from Phase 2 remains intact.

---

# Known Issues

1. The exact original logical partition-boundary construction algorithm is not sufficiently specified in the supplied paper material.

2. Logical partitions currently remain a graph-analysis abstraction and are not yet connected to the later task scheduler or HyTM cost model.

3. The partition target currently accounts for the CSR destination storage represented by the partition abstraction rather than every possible runtime metadata structure.

4. ExpTM-Filter currently produces a reference transfer plan rather than executing actual HyTGraph-specific CUDA host-to-device transfers.

5. Transfer-byte measurements represent logical partition payload rather than physical measured H2D traffic.

6. No benchmark measurements have yet been collected.

These are known limitations and do not block completion of the Phase 5 reference/filter baseline.

---

# Validation Status

The project owner reported the following local commands completed successfully:

    cmake -S . -B build -DHYTGRAPH_BUILD_TESTS=ON

    cmake --build build -j

    ctest --test-dir build --output-on-failure

Reported CTest result:

    100% tests passed, 0 tests failed out of 4

Total reported test time:

    1.79 sec

The reported CUDA algorithm tests executed successfully in the owner's local CUDA environment.

These results are recorded from user-provided output.

The assistant did not execute the build, tests, CUDA kernels, or benchmarks.

---

# Repository State

Phase 5 ExpTM-Filter is complete at the reference/filter-planning level.

The core filter behavior has been implemented and locally validated.

No known blocking compile, link, or test issue remains based on the reported local validation.

Actual HyTGraph-specific CUDA transfer execution remains for the appropriate later execution/integration phase.

---

# Current Boundary

The project should now move from:

    Phase 5 — ExpTM-Filter

to:

    Phase 6 — ExpTM-Compaction

Phase 6 should not be implemented prematurely as part of the Phase 5 filter.

ExpTM-Filter and ExpTM-Compaction remain separate mechanisms:

    ExpTM-Filter
        ↓
    select complete active logical partitions

    ExpTM-Compaction
        ↓
    compact active data within the selected transfer/execution scope

---

# Next Task

Phase 6 — ExpTM-Compaction.

Before coding Phase 6:

1. Inspect the Phase 6 requirements in `MASTER_PLAN.md`.
2. Locate the corresponding mechanism in the supplied HyTGraph paper.
3. Inspect the existing Phase 5 filter interfaces.
4. Determine how compaction should consume the filter/reference path.
5. Identify the role of Subway/CUB if required by the paper and master plan.
6. Preserve the Phase 5 correctness/reference path.
7. Implement only Phase 6.
8. Add/update tests.
9. Do not introduce later-phase mechanisms such as HyTM, VCGC, or multi-stream execution unless explicitly required by Phase 6.
