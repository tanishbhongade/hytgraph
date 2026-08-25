# HyTGraph Project State

## Current phase

**CURRENT PHASE:** Phase 0 — Infrastructure  
**CURRENT MILESTONE:** M0 — Repository/build/test infrastructure  
**CURRENT TASK:** Implement Phase 0 only  
**STATUS:** Complete, with CUDA compilation unverified because `nvcc` is not installed in the current environment.

## Implemented

- CMake/C++17 project structure.
- C++ runtime library containing configuration, logging, and result serialization.
- `hytgraph_dummy` executable for the Phase 0 structured-output smoke experiment.
- CTest unit-test infrastructure.
- Python experiment-runner skeleton using YAML configuration and JSON results.
- Versioned result schema at `docs/result_schema.json`.
- Dummy experiment configuration at `experiments/configs/dummy.yaml`.
- README with build/test/run instructions.
- Local Git repository initialized for the initially empty project.

## Files changed

- `CMakeLists.txt`
- `README.md`
- `MASTER_PLAN.md`
- `PROJECT_STATE.md`
- `.gitignore`
- `include/runtime/config.hpp`
- `include/runtime/logging.hpp`
- `include/runtime/result.hpp`
- `src/main.cpp`
- `src/runtime/config.cpp`
- `src/runtime/logging.cpp`
- `src/runtime/result.cpp`
- `tests/unit_tests.cpp`
- `experiments/run_experiment.py`
- `experiments/configs/dummy.yaml`
- `docs/result_schema.json`
- `cuda/smoke.cu`
- `datasets/.gitkeep`
- `results/.gitkeep`

## Interfaces changed

Initial project; no pre-existing interfaces existed.

Phase 0 public runtime interfaces:

- `hytgraph::runtime::Config`
- `hytgraph::runtime::LogLevel`, `set_log_level`, `log`
- `hytgraph::runtime::Result`, `result_to_json`, `write_result_json`
- `hytgraph_dummy` command-line interface
- `experiments/run_experiment.py` command-line interface

## Tests

- C++ unit tests cover typed configuration access, result JSON serialization,
  and result-file creation.
- CTest integration smoke test invokes the Python experiment runner and the
  dummy executable.
- `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release` succeeded.
- `cmake --build build -j2` succeeded.
- `ctest --test-dir build --output-on-failure` passed: 2/2 tests.
- The dummy experiment runner produced `results/phase0-dummy.json`.
- `cmake --build build --target hytgraph_cuda` succeeded via the explicit no-`nvcc` placeholder path.
- CUDA compilation was not run because `nvcc` is unavailable in this environment.

## Measurements

No HyTGraph performance measurements were produced in Phase 0.
The dummy result intentionally reports `dummy_runtime_seconds = 0.0`; this is
an infrastructure sentinel, not a performance measurement.

## Paper features

- [ ] CSR
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

- `nvcc` is not installed in the current execution environment, so only the
  CUDA target's fallback behavior was validated here. A CUDA-enabled machine
  must compile and run `cuda/smoke.cu` before claiming CUDA build validation.
- The C++ configuration object intentionally supports typed key/value settings,
  while YAML parsing is kept in the experiment runner. A full project-wide
  configuration model is deferred until later phases define the required
  algorithm/transfer/scheduling/cache parameters.
- The JSON schema is an initial infrastructure schema and is expected to gain
  phase-specific metric fields later without breaking the schema versioning
  approach.

## Paper fidelity

### Paper-derived

Phase 0 does not implement a HyTGraph processing mechanism. The infrastructure
is organized so later phases can incorporate the paper's stated dependencies:
SEP-Graph processing-kernel reference, neighbor shifting for ExpTM engines,
Subway-style CPU compaction, CUB for sorting/TopK/compaction, and CUDA-stream
execution.

### Engineering approximations

- No paper mechanism is approximated in Phase 0.
- The CUDA target fallback is an infrastructure/build accommodation, not a
  reproduction of CUDA execution.

### Not implemented

All graph-processing and HyTGraph optimization mechanisms are intentionally
left for later phases.

## Next task

**Phase 1 — CSR graph infrastructure**, only when explicitly requested.
