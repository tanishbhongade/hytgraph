# HyTGraph Reproduction Project — Current State

## Current Phase

Phase 6 — ExpTM-Compaction

## Current Milestone

M7 — ExpTM-Compaction

## Current Task

Implement and validate the Subway-style CPU ExpTM-Compaction reference baseline.

## Status

COMPLETE — reference CPU compaction baseline implemented and locally validated.

---

# Implemented

## Phase 0 — Infrastructure

Implemented:

- CMake/C++ project
- CUDA target infrastructure
- unit tests
- logging
- configuration
- experiment runner
- result schema

## Phase 1 — CSR Graph Infrastructure

Implemented:

- CSR graph storage
- graph loading
- graph validation
- degree queries
- neighbor queries

## Phase 2 — Correctness Reference Algorithms

Implemented:

- CPU PageRank
- CPU SSSP
- GPU baseline PageRank infrastructure
- GPU baseline SSSP infrastructure
- GPU/CPU correctness tests

## Phase 3 — Activity Tracking

Implemented:

- active vertex tracking
- active edge counting
- activity statistics

## Phase 4 — Logical Partitioning

Implemented:

- logical graph partitions
- configurable partition target
- CSR-row-preserving partition boundaries
- edge/storage accounting

## Phase 5 — ExpTM-Filter

Implemented:

- active partition detection
- whole-partition transfer planning
- inactive-partition skipping
- transferred edge accounting
- transferred byte accounting
- separate filter/reference path

## Phase 6 — ExpTM-Compaction

Implemented:

- Subway-style CPU/reference compaction baseline
- active source-vertex extraction
- compact neighbor payload
- compressed neighbor index
- inactive-source edge removal
- zero-degree active vertex handling
- all-active preservation path
- compaction byte accounting
- separate CPU compaction timing
- correctness/reference expansion path
- configurable enable/disable behavior
- validation of activity/graph compatibility
- validation of partition/CSR boundary consistency

---

# Files Changed

Phase 6 files:

- `include/transfer/compaction_engine.hpp`
- `src/transfer/compaction_engine.cpp`
- `CMakeLists.txt`
- `tests/unit_tests.cpp`

---

# Interfaces Changed

Added the ExpTM-Compaction interface:

- `ExpTMCompaction`
- `CompactedPartition`
- `CompactionResult`

Existing graph, activity, and logical-partition interfaces were preserved.

No Phase 5 filter interface was removed or redesigned.

---

# Tests

Phase 6 tests cover:

- basic compaction
- multiple active vertices
- active/inactive partition behavior
- zero-degree active vertices
- all-active graph preservation
- disabled/reference path
- activity/graph size mismatch
- invalid partition boundaries
- compacted neighbor ordering
- compressed index correctness
- byte accounting
- reference expansion

Local validation reported by the project owner:

    cmake --build build -j
    ctest --test-dir build --output-on-failure

Reported result:

    100% tests passed, 0 tests failed out of 3

Tests reported as passing:

- `unit_tests`
- `algorithm_tests`
- `experiment_runner_smoke`

CUDA was disabled for this validation run.

The assistant did not execute the build or tests.

---

# Measurements

Phase 6 records CPU-side compaction time separately from transfer execution.

The compacted representation records:

- active vertex count
- active edge count
- neighbor payload bytes
- compressed-index bytes
- total compacted bytes

No physical PCIe/H2D bandwidth or GPU execution timing is claimed.

No benchmark measurements have yet been collected.

---

# Paper Features

- [x] CSR
- [x] Partitioning
- [ ] SEP-Graph / equivalent GPU kernel
- [ ] Neighbor shifting
- [x] ExpTM-Filter
- [x] ExpTM-Compaction
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

# Dependencies

Phase 6 uses:

- `CSRGraph`
- `LogicalPartition`
- `LogicalPartitioner`
- `ActivityTracker`
- Phase 5 logical/reference transfer semantics

Phase 6 does not yet integrate:

- SEP-Graph
- CUB
- CUDA streams
- neighbor shifting
- HyTM
- VCGC
- ImpTM-Zero-Copy
- task combining
- contribution-driven scheduling

These remain later project mechanisms.

---

# Known Issues

1. The exact original logical partition-boundary construction algorithm is not sufficiently specified in the supplied paper material.

2. Logical partitions remain a graph-analysis abstraction and are not yet connected to the later task scheduler or HyTM cost model.

3. The partition target accounts for the CSR destination storage represented by the current partition abstraction rather than every possible runtime metadata structure.

4. ExpTM-Filter remains a reference transfer-planning implementation rather than actual HyTGraph-specific CUDA transfer execution.

5. ExpTM-Compaction is currently a CPU/reference implementation rather than a complete asynchronous HyTGraph execution pipeline.

6. The exact internal Subway implementation is not sufficiently specified to claim byte-for-byte reproduction.

7. Transfer-byte measurements represent logical/reference payloads rather than physical measured PCIe/H2D traffic.

8. No benchmark measurements have yet been collected.

---

# Paper Fidelity

## ExpTM-Filter

The reproduction follows the documented whole-partition filtering behavior:

- logical partitions are the filtering granularity;
- partitions containing active edges are selected;
- partitions containing no active edges are skipped;
- selected partitions are represented as complete logical-partition transfers;
- active-edge compaction is not performed by ExpTM-Filter.

## ExpTM-Compaction

The Phase 6 implementation follows the project specification for:

- CPU-assisted active-edge compaction;
- contiguous compacted neighbor storage;
- compressed neighbor/index representation;
- separate compaction measurement;
- correctness/reference behavior.

The implementation does not claim to reproduce undocumented internal details of the original HyTGraph/Subway runtime.

## Logical Partitioning

The reproduction uses:

- CSR vertex-row boundaries;
- configurable target-size balancing;
- no splitting of individual CSR adjacency rows;
- metadata-only logical partitions referencing the existing CSR graph.

---

# Engineering Approximations

## Logical Partitioning

The supplied paper material does not provide sufficient implementation detail to reconstruct the exact original partition-boundary construction algorithm.

The reproduction therefore uses CSR-row-preserving logical partitions.

## ExpTM-Compaction Representation

The compacted representation:

    active_vertices
    neighbor_index
    neighbors

is an engineering/reference representation.

It should not be interpreted as a confirmed byte-for-byte reconstruction of the original Subway internal representation.

## Runtime Execution

Phase 6 does not claim:

- GPU-side compaction execution;
- physical CUDA transfer timing;
- CUDA-stream overlap;
- SEP-Graph execution;
- neighbor-shifting execution.

Those mechanisms are separate project components.

---

# Validation Status

The project owner reported successful local validation after Phase 6 implementation.

Build:

    cmake --build build -j

CTest:

    ctest --test-dir build --output-on-failure

Reported:

    100% tests passed, 0 tests failed out of 3

The assistant did not execute these commands.

---

# Current Boundary

Phase 6 — ExpTM-Compaction is complete at the reference CPU-baseline level.

The architecture now progresses:

    CSR
      ↓
    Activity Tracking
      ↓
    Logical Partitioning
      ↓
    ExpTM-Filter
      ↓
    ExpTM-Compaction
      ↓
    Phase 7 — ImpTM-Zero-Copy

Phase 7 should remain separate from Phase 6.

Do not introduce HyTM, task combining, VCGC, or multi-stream execution prematurely.

---

# Next Task

Phase 7 — ImpTM-Zero-Copy
