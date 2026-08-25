# HyTGraph Reproduction

This repository is the incremental infrastructure for a credible reproduction
of the HyTGraph research system described in the supplied paper.

## Phase 0 status

Phase 0 provides only infrastructure:

- CMake/C++17 project structure
- a CUDA build target that becomes a real CUDA smoke-test executable when `nvcc`
  is available, and a clearly labeled placeholder otherwise
- CTest unit/integration smoke tests
- dependency-light C++ configuration handling
- logging
- a versioned JSON result schema
- a Python/YAML experiment runner skeleton
- a dummy experiment that emits structured JSON

No graph algorithm, partitioning, HyTM, task combining, scheduling, VCGC, or
multi-stream graph-processing mechanism is implemented in Phase 0.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Test

```bash
ctest --test-dir build --output-on-failure
```

## Run the dummy experiment

```bash
python3 experiments/run_experiment.py \
  --config experiments/configs/dummy.yaml \
  --output results/phase0-dummy.json \
  --executable build/hytgraph_dummy
```

The output is JSON and follows `docs/result_schema.json`.

## CUDA

The paper reports CUDA 10.1 on a GTX 2080 Ti test platform, but this repository
must remain buildable on development machines whose CUDA toolchain differs or
is absent. The Phase 0 CMake target therefore detects `nvcc` rather than
claiming a CUDA build was validated when it was not.

The supplied paper identifies SEP-Graph as the processing-kernel reference,
neighbor shifting as required for the explicit-transfer engines, CUB for
sorting/TopK/compaction operations, Subway-style CPU compaction, and multiple
CUDA streams. Those mechanisms are deliberately deferred to their planned
phases rather than being approximated in Phase 0.

## Source of truth

- `MASTER_PLAN.md` — project organization and development sequencing.
- Supplied HyTGraph paper — technical authority for HyTGraph behavior.
- Current codebase — authority for what has actually been implemented.
- `PROJECT_STATE.md` — persistent implementation handoff.
