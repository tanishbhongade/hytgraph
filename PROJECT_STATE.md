# HyTGraph Reproduction — Project State

## Current Phase

**Phase 7 — ImpTM-Zero-Copy**

## Current Milestone

**M8 — ImpTM-Zero-Copy**

## Current Task

Phase 7 paper/code fidelity review completed. ImpTM-Zero-Copy is complete at the reference/modeling level. The project is ready to begin Phase 8 — HyTM Cost Model.

## Status

**COMPLETE**

ImpTM-Zero-Copy request-count and alignment modeling has been implemented and locally validated. The implementation has been reviewed against the paper and the reproduction plan.

The implementation explicitly distinguishes:

- modeled zero-copy behavior
- alignment/request metrics
- actual mapped host-memory execution

No actual CUDA mapped/pinned-memory execution is claimed.

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

This is intentional. The Phase 8 cost model must combine them according to the paper's zero-copy cost equation.

---

# Phase 7 Files

The following files were added or modified for Phase 7:

    include/transfer/zero_copy_engine.hpp
    src/transfer/zero_copy_engine.cpp
    CMakeLists.txt
    tests/unit_tests.cpp

---

# Phase 7 Validation

The following project commands were run successfully by the repository owner:

    cmake --build build -j
    ctest --test-dir build --output-on-failure

Result:

    100% tests passed
    0 tests failed
    3 tests passed

CUDA remained disabled for this validation environment.

The Phase 7 tests cover:

- active-vertex request accounting
- active-edge accounting
- request payload configuration
- modeled TLP aggregation
- alignment overhead
- zero-degree active vertices
- invalid option handling
- invalid partition handling

---

# Phase 7 Important Implementation Details

## Request Payload

Default modeled request payload:

    128 bytes

Configurable through:

    ZeroCopyOptions::request_payload_bytes

## Maximum Outstanding Requests

Default:

    256 requests per TLP

Configurable through:

    ZeroCopyOptions::max_requests_per_tlp

## Alignment

Default logical alignment:

    128 bytes

Configurable through:

    ZeroCopyOptions::alignment_bytes

The current implementation uses the CSR row offset multiplied by the destination-entry size as a **logical address proxy**.

It does not claim this is the physical address of the mapped host allocation.

## Mode

The current result is explicitly:

    ZeroCopyMode::Modeled

and:

    host_memory_mapped == false

No actual CUDA host registration or mapped-memory allocation is performed.

---

# Phase 7 Paper Fidelity

The implementation is faithful to the paper at the intended reproduction/reference-model level.

The paper describes ImpTM-zero-copy as mapping pinned CPU memory into the GPU address space so the GPU can directly access CPU-resident data.

The reproduction currently models the resulting request behavior rather than implementing the complete CUDA mapped-memory lifecycle.

The paper's zero-copy request model and RTT model are reserved for the Phase 8 cost model.

The paper's engine-selection equations are also intentionally deferred to Phase 8.

---

# Architecture

Current reproduction architecture:

    CSR Graph
        |
        v
    Activity Tracking
        |
        v
    Logical Partitioning
        |
        +--------------------+
        |                    |
        v                    v
    ExpTM-Filter      ExpTM-Compaction
        |                    |
        |                    |
        +---------+----------+
                  |
                  v
           ImpTM-Zero-Copy
                  |
                  v
          Phase 8 — HyTM Cost Model

The transfer engines remain separate reference/modeling components.

---

# Phase 8 — HyTM Cost Model

**Status:** NOT STARTED

## Objective

Implement the HyTM cost model and deterministic transfer-engine selector described in the paper.

The implementation must remain scoped to the cost model and selector.

Do not implement later scheduling/task-combining phases yet.

---

# Phase 8 Requirements

Implement:

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

The selector must implement the paper's decision structure:

    if T_eci < alpha * T_efi
       and T_eci < beta * T_izi:
           choose ExpTM-Compaction

    else if T_izi < T_efi:
           choose ImpTM-Zero-Copy

    else:
           choose ExpTM-Filter

The comparisons must preserve the paper's strict `<` semantics.

---

# Phase 8 Cost Model

The cost model should operate independently for each logical partition.

## ExpTM-Filter

Model the filter transfer cost using:

    T_efi =
    ceil(
        transfer_bytes / m / MR
    ) * RTT

where:

- `m` = request payload size
- `MR` = maximum requests represented by a TLP
- `RTT` = modeled round-trip time

The partition's edge-data bytes are the reference transfer quantity.

---

## ImpTM-Zero-Copy

Per active vertex:

    requests(v) =
    ceil(Do(v) * d1 / m) + am(v)

Then:

    T_izi =
    ceil(
        sum(requests(v)) / MR
    ) * RTT_zc

The zero-copy RTT is:

    RTT_zc =
    gamma * RTT
    +
    (1 - gamma)
    * active_edge_proportion
    * RTT

with:

    gamma = 0.625

and:

    active_edge_proportion =
    active_edges / total_edges

for a partition with nonzero total edge count.

The implementation must avoid division by zero for empty partitions.

---

## ExpTM-Compaction

Reference transfer bytes:

    active_edge_bytes + active_vertex_index_bytes

The transfer component is:

    ceil(
        compaction_bytes / m / MR
    ) * RTT

The CPU compaction component is:

    compaction_bytes / T_hptcpt

Therefore:

    T_eci =
    transfer_time
    +
    CPU_compaction_time

The CPU compaction throughput must be configurable rather than inventing a paper-specific measured value that is not available in the current reproduction sources.

---

# Phase 8 Important Distinction

The Phase 7 zero-copy implementation intentionally reports:

    memory_request_count
    alignment_overhead_count

separately.

Phase 8 must use:

    memory_request_count + alignment_overhead_count

when evaluating the paper's zero-copy request equation.

The aggregate TLP metric from Phase 7 is therefore not by itself sufficient for the Phase 8 zero-copy cost calculation.

---

# Phase 8 Configuration

Expected model configuration should expose at least:

    alpha = 0.80
    beta  = 0.40
    gamma = 0.625

along with the transfer-model parameters required by the equations:

    request payload size
    maximum requests per TLP
    RTT
    CPU compaction throughput

The default values must be clearly labeled as model/reference defaults where the paper does not provide a directly reproducible runtime measurement.

---

# Phase 8 Tests

Tests should include deterministic synthetic cases covering:

1. **Compaction selected**
2. **Zero-copy selected**
3. **Filter selected**
4. Strict comparison boundaries
5. Default alpha/beta/gamma values
6. Zero-copy alignment overhead affecting the cost
7. Empty / zero-edge partition handling
8. Deterministic repeated selection
9. Invalid cost-model configuration
10. Per-partition independence

The tests must not depend on CUDA hardware.

---

# Phase 8 Scope Boundary

Phase 8 must NOT implement:

- four-filter-task grouping
- task combination
- CUDA kernel scheduling
- GPU-side selector execution
- Subway integration
- full asynchronous CPU/GPU pipeline
- benchmark reproduction
- later evaluation phases

Those belong to later phases of the reproduction plan.

---

# Known Issues / Engineering Approximations

The following are known and intentionally documented.

## 1. Exact Original Partition Boundaries

The paper does not provide enough information to reproduce every original runtime partition boundary exactly.

The project therefore uses deterministic logical partitions.

## 2. Logical Partitioning

Logical partitions currently serve as graph-analysis/reference abstractions.

They are not yet connected to a complete runtime scheduler.

## 3. Partition Target Size

The current partition target accounts for the graph's modeled destination/edge storage.

It does not claim to reproduce every runtime metadata allocation used by the original implementation.

## 4. ExpTM-Filter

The current implementation is a reference transfer model rather than the complete CUDA transfer mechanism.

## 5. ExpTM-Compaction

The current implementation is CPU/reference compaction.

It does not claim to reproduce the paper's complete asynchronous execution pipeline.

## 6. Subway

The exact internal Subway implementation and scheduling behavior are not fully specified by the available sources.

No unsupported implementation details are being invented.

## 7. Physical Transfer Accounting

Current transfer sizes are logical/reference byte counts.

They are not claimed to represent every physical PCIe transaction or runtime metadata transfer.

## 8. CUDA Execution

The current validation environment has CUDA disabled.

The CPU/reference implementation is therefore the primary reproducibility layer.

## 9. Zero-Copy Mapping

Phase 7 does not perform actual CUDA pinned host allocation, host registration, or mapped-memory pointer acquisition.

The behavior is explicitly modeled.

## 10. Zero-Copy Alignment

The zero-copy alignment calculation uses a logical CSR byte-offset proxy.

It is not a physical host-memory address calculation.

## 11. Zero-Copy TLP Metric

Phase 7 reports base request count and alignment overhead separately.

Phase 8 must combine them when applying the paper's zero-copy cost equation.

## 12. Compaction Throughput

A reproducible paper-specific CPU compaction throughput measurement is not currently available from the project sources.

Phase 8 should therefore expose throughput as a configurable model parameter.

## 13. No Benchmark Claims

No performance or benchmark equivalence to the original HyTGraph implementation is currently claimed.

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

Do not claim local execution unless the user provides the result.

---

# Repository Safety Rule

The repository is **read-only from the assistant's perspective**.

Do not write, modify, delete, or generate files directly inside the repository.

All implementation files must be provided as copy-pasteable content for the user to add manually.

---

# Current Validation Status

Last confirmed project validation:

    cmake --build build -j
    PASS

    ctest --test-dir build --output-on-failure
    PASS

    100% tests passed
    0 tests failed
    3 tests passed

CUDA:

    DISABLED

---

# Current Milestone

    M8 — ImpTM-Zero-Copy

Status:

    COMPLETE

---

# Next Milestone

    M9 — HyTM Cost Model

Status:

    READY TO START

---

# NEXT TASK

**Phase 8 — HyTM Cost Model**

First implementation step:

    Create include/transfer/hytm_cost_model.hpp

Provide the header only first. Do not implement the `.cpp`, CMake changes, or tests until the header has been added and validated locally.
