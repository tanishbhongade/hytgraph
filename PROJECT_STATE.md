# HyTGraph Project State

## Current phase

**CURRENT PHASE:** Phase 1 — CSR graph infrastructure
**CURRENT MILESTONE:** M1 — CSR
**CURRENT TASK:** Implement Phase 1 — CSR graph infrastructure
**STATUS:** Complete, pending final local verification after the explicit header-include cleanup.

## Implemented

### Phase 0

- CMake/C++17 project structure.
- C++ runtime library containing configuration, logging, and result serialization.
- `hytgraph_dummy` executable.
- CTest unit-test infrastructure.
- Python experiment-runner skeleton using YAML configuration and JSON results.
- Versioned result schema.
- Dummy experiment configuration.
- README/build/test/run infrastructure.
- CUDA target fallback for environments without `nvcc`.

### Phase 1

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

## Files changed

### Created

- `include/graph/csr_graph.hpp`
- `src/graph/csr_graph.cpp`
- `include/graph/graph_loader.hpp`
- `src/graph/graph_loader.cpp`

### Modified

- `CMakeLists.txt`
- `tests/unit_tests.cpp`
- `PROJECT_STATE.md`

## Interfaces

Phase 0 public interfaces remain unchanged.

Phase 1 adds:

- `hytgraph::graph::CSRGraph`
- `hytgraph::graph::GraphLoader`

## Tests

Phase 1 tests cover:

- basic CSR construction;
- vertex and edge counts;
- degree queries;
- neighbor queries;
- weighted CSR;
- invalid vertex queries;
- invalid CSR offsets;
- invalid destination IDs;
- invalid weight counts;
- unweighted graph loading;
- weighted graph loading;
- parallel edges;
- malformed input;
- invalid vertex IDs in input.

Local validation reported:

- `cmake --build build -j2` succeeded.
- `ctest --test-dir build --output-on-failure` passed.
- 2/2 CTest tests passed.
- `experiment_runner_smoke` continues to pass.

CUDA execution was not required for Phase 1.

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

## Algorithms

- [ ] PageRank
- [ ] SSSP
- [ ] BFS
- [ ] CC

## Datasets

None yet.

## Known issues

- The graph loader uses a simple directed edge-list format because the
  paper does not specify the reproduction project's input file format.
- Vertex IDs use 32-bit storage while CSR offsets use 64-bit storage, as
  specified by the reproduction project's architecture.
- A defensive upper-bound check for the number of vertices could be added
  later if required.
- CUDA execution remains unverified because `nvcc` is unavailable in the
  current environment.

## Paper fidelity

### Paper-derived

- HyTGraph uses CSR graph organization.
- Edge-associated graph data includes neighbor identities and weights.
- CSR-based organization supports later out-of-core and caching mechanisms.

### Engineering approximations

- C++ API design is specific to this reproduction project.
- The edge-list input format is a project-level engineering choice.
- Parallel-edge, ordering, and directed-graph semantics are explicitly defined
  by our loader because they are not specified by the paper.

## Not implemented

- Graph partitioning.
- Activity tracking.
- GPU graph algorithms.
- SEP-Graph-equivalent processing kernel.
- Transfer-management engines.
- HyTM.
- VCGC.
- Cache refresh.
- Contribution-driven scheduling.
- Multi-stream scheduling.

## Next task

**Phase 2 — Correctness reference algorithms**, only when explicitly requested.
