# HyTGraph Project State

## Current phase

**CURRENT PHASE:** Phase 2 — Correctness reference algorithms

**CURRENT MILESTONE:** M2 — CPU/GPU correctness references

**CURRENT TASK:** Implement Phase 2 — Correctness reference algorithms

**STATUS:** Complete — CPU and CUDA correctness paths locally verified.

---

## Implemented

### Phase 0 — Project infrastructure

- CMake/C++17 project structure.
- C++ runtime library containing configuration, logging, and result serialization.
- `hytgraph_dummy` executable.
- CTest unit-test infrastructure.
- Python experiment-runner skeleton using YAML configuration and JSON results.
- Versioned result schema.
- Dummy experiment configuration.
- README/build/test/run infrastructure.
- CUDA target fallback for environments without `nvcc`.

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

---

## Files changed for Phase 2

### Created

- `include/algorithms/pagerank.hpp`
- `include/algorithms/sssp.hpp`
- `src/algorithms/pagerank.cpp`
- `src/algorithms/sssp.cpp`
- `src/algorithms/gpu_stub.cpp`
- `cuda/pagerank.cu`
- `cuda/sssp.cu`
- `tests/algorithm_tests.cpp`
- `tests/cuda_algorithm_tests.cu`

### Modified

- `CMakeLists.txt`
- `PROJECT_STATE.md`

---

## Public interfaces

Phase 0 and Phase 1 public interfaces remain unchanged.

Phase 2 adds:

- `hytgraph::algorithms::PageRankOptions`
- `hytgraph::algorithms::PageRankResult`
- `hytgraph::algorithms::pagerank_cpu`
- `hytgraph::algorithms::pagerank_gpu`
- `hytgraph::algorithms::SSSPOptions`
- `hytgraph::algorithms::SSSPResult`
- `hytgraph::algorithms::sssp_cpu`
- `hytgraph::algorithms::sssp_gpu`

---

## Algorithm semantics

### PageRank

- Synchronous iterative reference implementation.
- Pull-style computation using a CPU-built incoming CSR representation.
- Configurable damping factor.
- Configurable convergence tolerance.
- Configurable maximum iteration count.
- Dangling-vertex probability mass is redistributed uniformly.
- GPU implementation follows the same correctness semantics as the CPU reference.

### SSSP

- Synchronous active-frontier relaxation.
- CPU and GPU implementations use snapshot-based update semantics.
- Edge weights are used when present.
- Unweighted graphs use unit edge weight.
- Finite non-negative edge weights are required.
- Source vertex is configurable.
- Maximum iterations are configurable.
- GPU implementation follows the same correctness semantics as the CPU reference.

---

## Tests

### Phase 1 / existing tests

- CSR construction and invariants.
- Vertex/edge counts.
- Degree queries.
- Neighbor queries.
- Weighted CSR.
- Invalid-query/CSR cases.
- Weighted/unweighted graph loading.
- Parallel edges.
- Malformed input.
- Invalid vertex IDs.

### Phase 2 CPU tests

- PageRank convergence on a directed cycle.
- PageRank dangling-vertex probability-mass preservation.
- PageRank option validation.
- Weighted SSSP known-answer graph.
- Unweighted SSSP.
- Unreachable-vertex behavior.
- SSSP negative-weight rejection.
- SSSP invalid-source rejection.

### Phase 2 CUDA tests

- GPU PageRank compared against CPU PageRank.
- GPU SSSP compared against CPU SSSP.

---

## Local validation

### CPU-only validation

Executed locally in the user's repository:

```text
cmake -S . -B build -DHYTGRAPH_ENABLE_CUDA=OFF
cmake --build build -j2
ctest --test-dir build --output-on-failure
```

Result:

```text
3/3 tests passed
100% tests passed, 0 tests failed
```

Validated tests:

- `unit_tests` — PASS
- `algorithm_tests` — PASS
- `experiment_runner_smoke` — PASS

### CUDA-enabled validation

Executed locally in the user's repository:

```text
cmake -S . -B build-cuda -DHYTGRAPH_ENABLE_CUDA=ON
cmake --build build-cuda -j2
ctest --test-dir build-cuda --output-on-failure
```

Result:

```text
4/4 tests passed
100% tests passed, 0 tests failed
Total Test time = 0.39 sec
```

Validated tests:

- `unit_tests` — PASS
- `algorithm_tests` — PASS
- `cuda_algorithm_tests` — PASS
- `experiment_runner_smoke` — PASS

The CUDA algorithm test actually executed successfully; it was not merely compiled.

---

## Build-system fixes

During local validation, a CMake issue was discovered where `cuda_algorithm_tests` could remain registered when:

```text
-DHYTGRAPH_ENABLE_CUDA=OFF
```

The test target incorrectly referenced:

```text
CUDA::cudart
```

without an available CUDA target.

The CMake logic was corrected so CUDA algorithm targets/tests are enabled only when CUDA is explicitly enabled and detected.

A clean CPU-only configuration now produces exactly the three non-CUDA tests.

---

## Compiler/toolchain notes

The CUDA compilation emitted repeated warnings of the form:

```text
warning: style of line directive is a GCC extension
```

These warnings occurred during NVCC compilation of the CUDA sources and generated stubs.

They did not prevent compilation or execution.

No CUDA source changes were made solely to suppress these warnings because the CUDA correctness tests passed successfully.

---

## Paper features

- [x] CSR
- [ ] Partitioning
- [ ] SEP-Graph/equivalent GPU kernel
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

- [x] PageRank reference
- [x] SSSP reference
- [ ] BFS
- [ ] CC

---

## Datasets

None yet.

---

## Paper fidelity

### Directly paper-derived

- CSR graph organization.
- Iterative vertex-centric computation model.
- PageRank and SSSP as primary algorithms/workloads.
- SSSP shortest-distance propagation from active vertices.
- GPU execution as the basis for later HyTGraph transfer-management mechanisms.

### Engineering approximations

The supplied paper does not specify every detail required for an independently executable correctness implementation.

#### PageRank

- Damping factor defaults to `0.85`.
- Convergence tolerance is configurable.
- Dangling-vertex mass is redistributed uniformly.

These choices are conventional engineering decisions and are **not claimed to be exact author parameters**.

#### SSSP

- Synchronous active-frontier relaxation is used for the correctness path.
- The paper does not specify every low-level detail of the reference implementation.

#### GPU

- The Phase 2 GPU implementation is a straightforward CUDA correctness baseline.
- It is **not** a reproduction of the full SEP-Graph execution mechanism.
- Incoming CSR is constructed for the pull-style correctness path.
- Explicit device memory copies are used.
- A simple synchronous execution model is used.
- CUDA streams, CUB, Subway, neighbor shifting, and transfer-management mechanisms are intentionally deferred.

---

## Not implemented

- Graph partitioning.
- Reusable activity-tracking subsystem.
- SEP-Graph-equivalent production kernel.
- Neighbor shifting.
- ExpTM-Filter.
- ExpTM-Compaction.
- ImpTM-Zero-Copy.
- HyTM.
- Task combining.
- Hub sorting.
- Contribution-driven scheduling.
- VCGC.
- Cache refresh.
- Multi-stream scheduling.
- BFS reference.
- Connected Components reference.
- Dataset integration.

---

## Known issues

1. PageRank's exact damping factor, convergence criteria, and dangling-node treatment are not specified in the supplied paper.
2. The Phase 2 GPU implementation is a correctness baseline rather than a SEP-Graph reproduction.
3. The CUDA compiler emitted GCC-extension warnings during compilation, although the resulting executable compiled and passed its correctness test.
4. Incoming CSR introduces additional memory/storage for the Phase 2 correctness path.
5. Activity tracking is currently internal to the SSSP implementation rather than exposed as the reusable Phase 3 subsystem.
6. No transfer-management mechanism has been implemented yet.
7. No performance benchmarks have been established yet.
8. No HyTGraph-specific optimization claims should be made from the Phase 2 implementation.

---

## Phase 2 exit criteria

The Phase 2 CPU/GPU correctness requirement is satisfied locally:

```text
CPU PageRank        PASS
CPU SSSP            PASS
GPU PageRank        PASS
GPU SSSP            PASS
Existing tests      PASS
Experiment runner   PASS
```

Therefore:

```text
M2 — CPU/GPU correctness references: COMPLETE
```

---

## Next task

**Phase 3 — Activity tracking**

Implement only the activity-tracking subsystem required by the master plan, including active-vertex/active-edge representation and hand-verifiable activity statistics.
