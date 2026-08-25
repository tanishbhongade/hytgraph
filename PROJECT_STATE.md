# HyTGraph Project State

## Current phase

**CURRENT PHASE:** Phase 3 — Activity tracking

**CURRENT MILESTONE:** M3 — Activity tracking reference

**CURRENT TASK:** Implement Phase 3 — Activity tracking

**STATUS:** Complete — reusable CPU activity tracking implemented and locally verified.

---

## Implemented

### Phase 0 — Project infrastructure

- CMake/C++17 project structure.
- CUDA target fallback for environments without CUDA.
- C++ runtime library containing configuration, logging, and result serialization.
- `hytgraph_dummy` executable.
- CTest unit-test infrastructure.
- Python experiment-runner skeleton using YAML configuration and JSON results.
- Versioned result schema.
- Dummy experiment configuration.
- README/build/test/run infrastructure.

### Phase 1 — CSR graph infrastructure

- CSR graph storage.
- 64-bit CSR row offsets.
- 32-bit destination vertex IDs.
- Optional `float` edge weights.
- CSR invariant validation.
- Vertex degree queries.
- Neighbor range queries.
- Individual neighbor queries.
- Individual edge-weight queries.
- Directed edge-list graph loader.
- Weighted and unweighted graph loading.
- Comment and blank-line handling.
- Parallel-edge preservation.
- Malformed-input validation.
- CSR unit tests.
- Graph-loader unit tests.
- CMake integration of graph sources.

### Phase 2 — Correctness reference algorithms

#### CPU

- CPU PageRank correctness/reference implementation.
- CPU SSSP correctness/reference implementation.
- Configurable PageRank damping factor, convergence tolerance, and maximum iterations.
- Synchronous PageRank computation.
- CPU PageRank dangling-vertex mass redistribution.
- Synchronous active-frontier SSSP.
- Weighted SSSP.
- Unweighted SSSP using unit edge weights.
- Validation of SSSP source and edge weights.

#### CUDA

- GPU PageRank baseline implementation.
- GPU SSSP baseline implementation.
- CUDA-unavailable stub implementations.
- CPU/GPU correctness comparison tests.
- Explicit device memory management.
- Single-stream/synchronous baseline execution.
- GPU-side PageRank and SSSP iterative kernels.

#### Build/test integration

- `hytgraph_algorithms` library.
- CPU algorithm test target.
- Conditional CUDA algorithm test target.
- CUDA test registration only when CUDA is actually enabled and available.
- Existing Phase 0/Phase 1 interfaces preserved.

### Phase 3 — Activity tracking

- Reusable `ActivityTracker` subsystem.
- Active-vertex representation using a byte-per-vertex activity state.
- Explicit active-vertex count.
- Individual vertex activation/deactivation.
- Replacement of the complete active-vertex set.
- Deterministic active-vertex enumeration.
- Active-edge calculation from active source vertices and CSR out-degrees.
- Partition activity statistics over externally supplied vertex ranges.
- Per-partition active-vertex count.
- Per-partition active-edge count.
- Per-partition total-edge count.
- Active-vertex and active-edge presence checks.
- Activity-state validation.
- Graph/activity vertex-count validation.
- Partition-range validation.
- SSSP migrated from its private activity vectors to the reusable `ActivityTracker`.
- Existing SSSP public interface preserved.

---

## Files changed for Phase 3

### Created

- `include/graph/activity_tracker.hpp`
- `src/graph/activity_tracker.cpp`

### Modified

- `CMakeLists.txt`
- `src/algorithms/sssp.cpp`
- `tests/algorithm_tests.cpp`
- `PROJECT_STATE.md`

---

## Public interfaces

Phase 0 and Phase 1 public interfaces remain unchanged.

Phase 2 public interfaces remain unchanged:

- `hytgraph::algorithms::PageRankOptions`
- `hytgraph::algorithms::PageRankResult`
- `hytgraph::algorithms::pagerank_cpu`
- `hytgraph::algorithms::pagerank_gpu`
- `hytgraph::algorithms::SSSPOptions`
- `hytgraph::algorithms::SSSPResult`
- `hytgraph::algorithms::sssp_cpu`
- `hytgraph::algorithms::sssp_gpu`

Phase 3 adds:

- `hytgraph::graph::ActivityTracker`
- `hytgraph::graph::ActivityTracker::VertexRange`
- `hytgraph::graph::ActivityTracker::PartitionActivity`

The SSSP public API was not changed.

---

## Activity semantics

The Phase 3 activity tracker follows the paper's active-subgraph semantics:

- Active vertices represent vertices participating in the current active frontier.
- Outgoing edges of active vertices constitute the active-edge set.
- Active-edge volume is therefore computed as the sum of `Do(v)` over active vertices.
- Partition activity statistics are computed independently for supplied vertex ranges.

The paper does not specify the exact C++ representation or API of the activity tracker. The reusable C++ interface and byte-per-vertex representation are therefore implementation choices.

---

## SSSP activity integration

SSSP previously maintained its activity state internally.

Phase 3 replaces that private representation with:

```text
ActivityTracker
```
