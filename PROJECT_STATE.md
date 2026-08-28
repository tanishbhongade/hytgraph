# HyTGraph Reproduction — Project State

## Current Phase

Phase 4 — Logical Partitioning

## Current Milestone

M5 — Partitioning

## Current Task

Implement logical graph partitioning for fine-grained partition-level analysis.

## Status

Phase 4 logical partitioning implementation is complete.

The implementation has been added, integrated into the CMake build, and tested locally by the project owner.

The local build completed successfully and the CTest suite passed.

---

## Implemented Phases

### Phase 0 — Project Infrastructure

Implemented:

- CMake project structure
- Runtime library
- Test infrastructure
- CUDA build target
- Configuration infrastructure
- Logging/result infrastructure

### Phase 1 — CSR Graph Infrastructure

Implemented:

- CSR graph representation
- CSR validation
- Graph loading
- Graph-related unit tests

### Phase 2 — Reference Algorithms

Implemented:

- CPU reference algorithms
- GPU algorithm infrastructure
- PageRank
- SSSP

### Phase 3 — Activity Tracking

Implemented:

- Reusable CPU ActivityTracker
- Active/inactive edge tracking
- Partition-statistics interface using vertex ranges
- Integration with SSSP

### Phase 4 — Logical Partitioning

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

---

## Phase 4 Implementation

Logical partitions are represented as metadata over the existing CSR graph.

A partition contains:

- `[vertex_begin, vertex_end)`
- `[edge_begin, edge_end)`
- configured target partition size

The partitioner walks CSR vertex rows and accumulates the corresponding edge storage.

A partition boundary is created before a vertex when adding that vertex would cause the current non-empty partition to exceed the configured target size.

CSR rows are never split.

Therefore the configured partition size is a target rather than an exact size.

A vertex with an adjacency list larger than the target may produce an oversized partition.

---

## Phase 4 Configuration

Default logical partition size:

    32 MiB

Equivalent byte value:

    32 * 1024 * 1024

The partitioner also accepts a custom byte target through its constructor.

---

## Phase 4 Correctness Invariants

The implementation verifies/assumes the following invariants:

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

---

## Files Added/Modified for Phase 4

Added:

- `include/graph/partition.hpp`
- `src/graph/partition.cpp`

Modified:

- `CMakeLists.txt`
- `tests/unit_tests.cpp`

---

## Tests

Phase 4 tests cover:

- Basic logical partitioning
- Default partition size
- Custom partition size
- Empty graph
- Zero-degree vertices
- Oversized vertex adjacency lists
- Invalid partition size
- Partition vertex-boundary correctness
- Partition edge-boundary correctness
- Full vertex coverage
- Full edge coverage

The project owner reported that the local build and complete CTest suite passed after the Phase 4 changes.

No benchmark measurements have been collected.

---

## Paper Features

- [x] CSR
- [x] Logical partitioning
- [ ] SEP-Graph / equivalent GPU kernel
- [ ] Neighbor shifting
- [ ] ExpTM-Filter
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

## Algorithms

- [x] PageRank
- [x] SSSP
- [ ] BFS
- [ ] Connected Components

---

## Datasets

No benchmark dataset has been integrated yet.

---

## Paper Fidelity

The following aspects directly follow the supplied HyTGraph paper/master plan:

- Logical partitioning is used for fine-grained graph analysis.
- Logical partitions use a 32 MB target.
- Logical partitioning is distinct from later execution-task granularity.
- Partition size is configurable in the reproduction project.

---

## Engineering Approximations

The supplied paper material does not provide sufficient implementation detail to reconstruct the exact original partition-boundary construction algorithm.

The reproduction therefore uses:

- CSR vertex-row boundaries.
- Approximate target-size balancing based on CSR destination storage.
- No splitting of individual CSR adjacency rows.
- Metadata-only logical partitions referencing the existing CSR graph.

These choices are engineering approximations and should not be interpreted as confirmed details of the original HyTGraph implementation.

The paper's 32 MB terminology is represented as 32 MiB / 33,554,432 bytes in the implementation.

---

## Dependencies

Phase 4 does not yet integrate:

- SEP-Graph
- Subway
- CUB
- CUDA streams
- Neighbor shifting
- ExpTM-Filter
- ExpTM-Compaction
- VCGC
- HyTM

These belong to later phases.

ActivityTracker already provides partition-statistics functionality over vertex ranges and therefore remains compatible with the logical partition representation.

---

## Known Issues

1. The exact partition-boundary construction algorithm used by the original HyTGraph implementation is not specified sufficiently in the supplied paper material.

2. Logical partitions currently exist as a graph-analysis abstraction. They are not yet connected to the later transfer engine, task scheduler, or HyTM cost model.

3. The implementation currently accounts for CSR destination-array bytes when evaluating the partition target. It does not treat every graph/runtime metadata structure as part of the partition byte target.

4. No GPU execution or performance measurement has been performed for Phase 4.

---

## Validation Status

The project owner reported:

    cmake --build build -j

completed successfully.

The project owner also reported:

    ctest --test-dir build --output-on-failure

completed successfully with all tests passing.

These results were not executed by the assistant and are recorded as user-provided validation.

---

## Current Repository State

Phase 4 logical partitioning is implemented and integrated into the build.

No known blocking compile, link, or test issue remains for Phase 4 based on the reported local validation.

---

## Next Task

Phase 5 — ExpTM-Filter

Before implementing Phase 5:

1. Inspect the current transfer/runtime architecture.
2. Locate the ExpTM-Filter mechanism in the paper.
3. Determine how logical partitions connect to transfer decisions.
4. Identify the existing transfer abstraction, if any.
5. Implement only the ExpTM-Filter portion required by the master plan.
6. Preserve a correctness/reference path.
7. Add tests before moving to later ExpTM phases.
