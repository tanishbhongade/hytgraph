# HyTGraph Reproduction Project — Master Implementation Plan

**Final version for multi-chat, LLM-assisted implementation**

## Project goal

Build a credible, modular reproduction of the HyTGraph paper, targeting approximately **60–70% of the original system's performance improvement** where hardware differences permit meaningful comparison. Exact hardware-level timing reproduction is not required.

---

# 1. How this document is used

This is the persistent project specification. Supply it to every new coding LLM chat together with:

1. the original HyTGraph paper;
2. the current codebase;
3. the latest project-state handoff;
4. one specific implementation task.

### Source of truth

- **Original paper** = technical/research authority for what HyTGraph does.
- **Master plan** = project authority for how our reproduction is organized.
- **Codebase** = authority for what is actually implemented.
- **Current chat** = authority for the specific task being performed.

### Source-priority rule

If sources appear to conflict:

1. Do not silently invent or choose a paper detail.
2. Identify the conflict.
3. Check the relevant paper section.
4. Preserve the master-plan decision only when it is explicitly an engineering choice/approximation.
5. Document any deviation from the paper.

## 1.1 Non-negotiable LLM rules

1. Do not implement the entire project at once.
2. Implement only the requested phase/task.
3. Do not silently redesign the architecture.
4. Do not invent paper details.
5. Do not present an engineering approximation as an exact reproduction.
6. Do not present modeled hardware behavior as measured hardware behavior.
7. Preserve existing interfaces unless a change is necessary.
8. Add tests for non-trivial functionality.
9. Keep a working correctness/reference implementation whenever possible.
10. Every optimization must be independently disableable for ablation.
11. Every optimization must have measurable metrics.
12. Every asynchronous implementation must retain a correctness-checkable reference path.
13. Every experiment must record its configuration and software version.
14. Every coding chat must finish with a project-state handoff.
15. If the paper and the plan conflict, explicitly flag the conflict.

---

# 2. Core project objective

The project is not intended to reproduce every line of the authors' source code or identical absolute runtimes. It should reproduce the core mechanisms, behavior, experimental questions, and qualitative/quantitative trends of HyTGraph.

The implementation should provide:

- Correct graph-processing results.
- Working HyTM with the three transfer strategies.
- Working task combining.
- Working contribution-driven scheduling.
- Working vertex-centric graph caching.
- Measurable communication reduction.
- Paper-aligned ablations.
- A reproducible experiment pipeline.

---

# 3. HyTGraph mechanisms to reproduce

```text
CSR graph
   ↓
Edge-balanced logical partitions
   ↓
Active vertex / active edge tracking
   ↓
HyTM cost analysis
   ↓
┌────────────────┬──────────────────┬─────────────────┐
│ ExpTM-Filter   │ ExpTM-Compaction │ ImpTM-Zero-Copy│
└────────────────┴──────────────────┴─────────────────┘
   ↓
Task Combining
   ↓
Contribution-Driven Scheduling
   ↓
SEP-Graph execution layer (vendored from original HyTGraph repo)
   ↓
GPU computation
   ↓
Vertex-Centric Graph Caching
   ↓
Next iteration
```

---

# 4. Important paper implementation dependencies

These must not be forgotten.

| Dependency             | Requirement                                                                                                                                                                                                                                                                                                                                                                                           |
| ---------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **SEP-Graph + Groute** | HyTGraph uses the original authors' vendored copy of SEP-Graph and Groute as its GPU execution layer. This reproduction **reuses the same vendored code** (unmodified) from the `iDC-NEU/HyTGraph` GitHub repository as a separate CMake library under `third_party/hytgraph_sep/`. All interaction with the vendored code happens exclusively through `include/sep_adapter/` and `src/sep_adapter/`. |
| **Subway**             | CPU active-edge compaction follows the Subway design and regenerates a compressed neighbor/index representation.                                                                                                                                                                                                                                                                                      |
| **CUB**                | Paper implementation uses CUB for sorting, TopK, and compaction where applicable. CUB is already vendored inside the original HyTGraph repo.                                                                                                                                                                                                                                                          |
| **CUDA streams**       | Multiple streams overlap GPU computation, transfers, and CPU compaction. Streams are obtained from `groute::Stream` (part of the vendored code).                                                                                                                                                                                                                                                      |
| **Neighbor shifting**  | Must be considered when implementing the explicit-transfer engines. SEP-Graph's `GraphDatum` supports shifted representations.                                                                                                                                                                                                                                                                        |

If an exact dependency cannot be integrated, implement the closest defensible equivalent and label it explicitly as an **engineering approximation**.

---

# 5. Target algorithms

### Primary

- PageRank
- SSSP

### Secondary

- BFS
- Connected Components

VCGC should be disabled for BFS when following the paper's design because BFS does not provide the same cross-layer reuse opportunity.

---

# 6. Recommended architecture

```text
hytgraph/
├── CMakeLists.txt
├── README.md
├── MASTER_PLAN.md
├── include/
│   ├── graph/{csr_graph.hpp,graph_loader.hpp,partition.hpp}
│   ├── algorithms/{pagerank.hpp,sssp.hpp,bfs.hpp,cc.hpp}
│   ├── transfer/{transfer_engine.hpp,filter_engine.hpp,compaction_engine.hpp,zero_copy_engine.hpp,cost_model.hpp}
│   ├── scheduling/{task.hpp,task_combiner.hpp,contribution_scheduler.hpp,hub_sort.hpp}
│   ├── cache/{vertex_cache.hpp,hotness.hpp,cache_refresh.hpp}
│   ├── runtime/{execution_context.hpp,metrics.hpp,config.hpp,pipeline_config.hpp}
│   └── sep_adapter/
│       ├── sep_variant.hpp
│       ├── sep_execution_result.hpp
│       ├── sep_execution_driver.hpp
│       ├── sep_variant_mapper.hpp
│       ├── sep_engine_adapter.hpp
│       ├── sep_engine_factory.hpp
│       ├── null_sep_driver.hpp
│       ├── sep_graph_datum_adapter.hpp
│       ├── hytm_sep_bridge.hpp
│       ├── sep_task_combiner.hpp
│       ├── sep_worklist_adapter.hpp
│       ├── sep_contribution_scheduler.hpp
│       ├── sep_priority_buffer.hpp
│       ├── sep_vcgc_integration.hpp
│       ├── sep_cached_graph_view.hpp
│       ├── sep_vcgc_refresh.hpp
│       ├── sep_stream_manager.hpp
│       ├── sep_stream_scheduler.hpp
│       └── full_pipeline.hpp
├── src/
│   └── sep_adapter/
├── cuda/
├── third_party/
│   └── hytgraph_sep/
│       ├── include/{framework,groute,utils}/
│       ├── deps/
│       └── CMakeLists.txt
├── tests/
├── experiments/{configs,run_experiment.py,collect_results.py,plot_results.py}
├── datasets/
├── results/
└── docs/
```

### Integration rule

The original HyTGraph SEP-Graph + Groute code lives **only** under `third_party/hytgraph_sep/` and is compiled as its own CMake target. Nothing in `include/` or `src/` may include vendored headers directly — all access goes through `include/sep_adapter/`. This preserves a clean boundary and lets the vendored library be replaced or upgraded independently.

**The `third_party/hytgraph_sep/` directory is read-only, forever.** All changes go in `sep_adapter/`.

---

# 7. Development phases

## Phase 0 — Infrastructure

Implement:

- CMake/C++ project
- CUDA target
- tests
- logging
- configuration
- experiment runner
- result schema

**Exit criterion:** builds/tests run; dummy experiment produces structured output.

## Phase 1 — CSR graph infrastructure

Implement:

- CSR storage
- graph loader
- validation
- degree/neighbor queries
- optional weights

**Exit criterion:** CSR invariants and loader tests pass.

## Phase 2 — Correctness reference algorithms

Implement:

- CPU PageRank
- CPU SSSP
- GPU baseline PageRank
- GPU baseline SSSP

**Exit criterion:** GPU results agree with CPU reference within documented tolerances.

## Phase 3 — Activity tracking

Implement:

- active vertices
- active edges
- partition activity statistics

**Exit criterion:** activity counts match hand-verified graphs.

## Phase 4 — Logical partitioning

Implement:

- edge-balanced logical partitions
- 32 MB paper-aligned default
- configurable partition size

**Exit criterion:** partition counts and edge totals are correct.

## Phase 5 — ExpTM-Filter

Implement:

- full active-partition transfer
- skip inactive partitions
- transfer/computation measurements

**Exit criterion:** correct and measurable filter baseline.

## Phase 6 — ExpTM-Compaction

Implement:

- Subway-style CPU compaction
- compressed neighbor/index representation
- separate compaction measurement

**Exit criterion:** correct and measurable compaction baseline.

## Phase 7 — ImpTM-Zero-Copy

Implement:

- mapped/pinned host-memory path where supported
- modeled fallback when necessary
- request/alignment metrics

**Exit criterion:** actual vs modeled behavior is clearly labeled.

## Phase 8 — HyTM cost model

Implement:

- paper equations
- α = 0.80
- β = 0.40
- γ = 0.625 for zero-copy RTT model
- unit-tested engine selector

**Exit criterion:** synthetic partition cases produce valid deterministic selections.

## Phase 9 — Task combining

Implement:

- filter `k = 4` consecutive partitions
- combined compaction tasks
- combined zero-copy tasks
- task-count reduction measurement

**Exit criterion:** logical decisions remain unchanged while executable task count falls.

## Phase 10 — Hub sorting

Implement:

```text
H(v) = Do(v) × Di(v) / (Do_max × Di_max)
```

Use approximately the top **8%** important vertices and group them at the beginning of CSR ordering.

**Exit criterion:** ordering and hub-score tests pass; scheduling/HyTM effects are measurable.

## Phase 11 — Contribution-driven scheduling

Implement:

- PageRank contribution/delta priority
- SSSP contribution-aware priority
- synchronous reference
- stale/redundant work metrics

**Exit criterion:** correctness is preserved and scheduling impact is measurable.

## Phase 12 — Organize the working HyTGraph SEP-Graph + Groute code

**Prerequisite:** The original HyTGraph repository builds and runs on the developer's machine. The exact working configuration is documented in `docs/original_hytgraph_build_notes.md`.

**Goal:** Move the working vendored code into the project as an unmodified, separately-buildable CMake library, without introducing any new build risk.

### Detailed steps

1. **Capture the working state first.**
   - Write `docs/original_hytgraph_build_notes.md` documenting:
     - CUDA version, driver version, GPU model, OS.
     - CMake version, GCC version.
     - Every source or CMake edit made to the original repo.
     - Every dependency installed (CUB, gflags, etc.).
     - Exact build command that succeeded.
     - Exact command that runs each sample successfully.
   - Take a snapshot: `nvcc --version`, `nvidia-smi`, `cmake --version`, `gcc --version`.

2. **Establish a known-good reference.**
   - Run the original repo's samples (`hybrid_pr`, `hybrid_sssp`, `hybrid_bfs`, `hybrid_cc`) on a small graph.
   - Record command, output, runtime, and graph used.
   - Store this in `docs/original_hytgraph_reference_runs.md`.
   - This becomes the ground-truth reference for all later phases.

3. **Copy the working code into the project.**
   - Copy `include/framework/`, `include/groute/`, `include/utils/`, and `deps/` into `third_party/hytgraph_sep/`.
   - Apply the _same_ fixes documented in step 1.
   - Do not clean up, do not modernize, do not reorder.
   - Keep the original working copy untouched as a gold reference.

4. **Create the CMake wrapper.**
   - `third_party/hytgraph_sep/CMakeLists.txt` produces `hytgraph_sep_lib`.
   - Wire into root CMake behind `HYTGRAPH_WITH_SEP_GRAPH` option:
     ```
     option(HYTGRAPH_WITH_SEP_GRAPH "Build with vendored HyTGraph SEP-Graph" ON)
     if(HYTGRAPH_WITH_SEP_GRAPH)
         add_subdirectory(third_party/hytgraph_sep)
     endif()
     ```
   - Do not edit upstream `.cuh`/`.h`/`.cu` files.

5. **Add the smoke test.**
   - `tests/integration/sep_graph_link_smoke_test.cpp`.
   - Include one vendored header (e.g. `framework/algo_variants.cuh`).
   - Instantiate a `sepgraph::common::AlgoVariant` value; round-trip it.
   - Do not launch kernels.

6. **Verify no leakage.**
   - Grep `include/` and `src/` for `sepgraph::` and `groute::`.
   - Only allowed hit: the smoke test.

7. **Verify the vendored library still builds in the new tree.**
   - Confirm `hytgraph_sep_lib` compiles.
   - Confirm the smoke test links.
   - Confirm all Phase 0–11 tests still pass.

### Time-box

**2 days.** This is organizational work, not research work.

### Deliverables

- `third_party/hytgraph_sep/` populated with the working original code.
- `third_party/hytgraph_sep/UPSTREAM_REVISION.txt`.
- Root `CMakeLists.txt` with `HYTGRAPH_WITH_SEP_GRAPH` option.
- `tests/integration/sep_graph_link_smoke_test.cpp`.
- `docs/original_hytgraph_build_notes.md`.
- `docs/original_hytgraph_reference_runs.md`.

### Exit criterion

- `hytgraph_sep_lib` builds on the target platform.
- Smoke test compiles and links.
- All Phase 0–11 tests still pass.
- No `sepgraph::` or `groute::` symbol appears outside `third_party/hytgraph_sep/` and the smoke test.
- The original repo's samples still run and produce the recorded reference output.

## Phase 13 — Adapter layer (no data movement)

**Goal:** Introduce the `sep_adapter` module as the single boundary between project code and vendored code. Deliver a thin wrapper around the engine.

### New files

- `include/sep_adapter/sep_variant.hpp`
- `include/sep_adapter/sep_execution_result.hpp`
- `include/sep_adapter/sep_execution_driver.hpp`
- `include/sep_adapter/sep_variant_mapper.hpp` + `src/sep_adapter/sep_variant_mapper.cpp`
- `include/sep_adapter/sep_engine_adapter.hpp` + `src/sep_adapter/sep_engine_adapter.cpp`
- `include/sep_adapter/sep_engine_factory.hpp` + `src/sep_adapter/sep_engine_factory.cpp`
- `include/sep_adapter/null_sep_driver.hpp`

### Public interface (project-owned)

```cpp
namespace hytgraph::sep_adapter {

enum class SEPVariant {
    SYNC_PUSH_DD, SYNC_PULL_DD, SYNC_PUSH_TD, SYNC_PULL_TD,
    ASYNC_PUSH_DD, ASYNC_PULL_DD, ASYNC_PUSH_TD, ASYNC_PULL_TD,
};

struct SEPExecutionResult {
    enum class State { OK, NEED_INIT, INVALID_CONFIG, DEFERRED, FAILED };
    State       state = State::OK;
    std::string message;
};

class SEPExecutionDriver {
public:
    virtual ~SEPExecutionDriver() = default;
    virtual SEPExecutionResult initialize() = 0;
    virtual SEPExecutionResult execute_step() = 0;
    virtual SEPExecutionResult synchronize() = 0;
    virtual SEPVariant variant() const noexcept = 0;
    virtual bool       is_initialized() const noexcept = 0;
};

template <typename TApp>
class SEPEngineAdapter final : public SEPExecutionDriver {
public:
    SEPEngineAdapter(const TApp& app, SEPVariant variant);
    ~SEPEngineAdapter() override;
    SEPExecutionResult initialize() override;
    SEPExecutionResult execute_step() override;
    SEPExecutionResult synchronize() override;
    SEPVariant variant() const noexcept override;
    bool       is_initialized() const noexcept override;
    void* native_engine_handle() noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace hytgraph::sep_adapter
```

### Implementation rules

- pimpl everywhere so `sepgraph::` and `groute::` never leak into public headers.
- `variant_mapper.cpp` is the only file that includes `algo_variants.cuh`.
- `initialize()` constructs `sepgraph::engine::Engine<TApp>`; confirm the single-step method name against `framework.cuh`.
- Null driver returns `DEFERRED` and allows the abstraction to be tested without vendored code.

### Tests

- Round-trip every enum value through `to_string`/`from_string`.
- Mapping is bijective with `sepgraph::common::AlgoVariant`.
- Null driver behaves as documented.
- Factory produces a driver for every `(algorithm, variant)` pair.
- Toy app (4 vertices) executes one step.

### Exit criterion

- Toy app runs one SEP step through the adapter.
- Adapter `.hpp` files contain no vendored symbols.
- Phase 0–12 tests still pass.

## Phase 14 — Data-movement bridge

**Goal:** Feed our `CSRGraph` + partition into the engine.

### New files

- `include/sep_adapter/sep_graph_datum_adapter.hpp` + `src/sep_adapter/sep_graph_datum_adapter.cpp`
- `include/sep_adapter/hytm_sep_bridge.hpp` + `src/sep_adapter/hytm_sep_bridge.cpp`

### Public interface

```cpp
namespace hytgraph::sep_adapter {

enum class TransferEngineType {
    ExpTMFilter, ExpTMCompaction, ImpTMZeroCopy, None
};

struct BridgeConfig {
    std::size_t batch_hint = 0;
    bool enable_neighbor_shift = true;
};

class HyTMSEPBridge {
public:
    HyTMSEPBridge(SEPExecutionDriver& driver,
                  const CSRGraph& graph,
                  BridgeConfig config = {});
    SEPExecutionResult execute_partition(
        const std::vector<uint32_t>& active_vertices,
        TransferEngineType engine);
    const std::vector<uint32_t>& last_active_output() const noexcept;
private:
    SEPExecutionDriver&   driver_;
    const CSRGraph&       graph_;
    BridgeConfig          config_;
    std::vector<uint32_t> last_active_output_;
};

struct GraphDatumPayload;  // pimpl
std::unique_ptr<GraphDatumPayload>
build_graph_datum_payload(const CSRGraph& graph,
                          const std::vector<uint32_t>& active_vertices);

} // namespace hytgraph::sep_adapter
```

### Bridging logic

- Map `row_offsets` → `groute::graphs::host::CSRGraph::offsets`.
- Map `column_indices` → `groute::graphs::host::CSRGraph::edges`.
- Map `edge_weights` → `groute::graphs::host::CSRGraph::weights`.
- Use `CSRGraphAllocator` to move to device.
- Slice to active vertex set; wrap in `sepgraph::graphs::GraphDatum`.
- Sort and deduplicate the active set once per step.
- Reuse the engine across partitions; only swap the `GraphDatum` input.
- Apply neighbor shifting when engine is ExpTMFilter/ExpTMCompaction.

### Tests

- CSR slice correctness for empty, full, and partial active sets.
- Single out-of-core iteration via each of the three transfer engines.
- Results match CPU reference within `1e-4`.

### Exit criterion

- One out-of-core iteration runs through the vendored engine.
- All three transfer engines give identical results.
- No `sepgraph::` symbol visible outside `src/sep_adapter/`.

## Phase 15 — Task combining bridge

**Goal:** Feed `TaskCombiner` output into the vendored worklist.

### New files

- `include/sep_adapter/sep_task_combiner.hpp` + `src/sep_adapter/sep_task_combiner.cpp`
- `include/sep_adapter/sep_worklist_adapter.hpp` + `src/sep_adapter/sep_worklist_adapter.cpp`

### Public interface

```cpp
namespace hytgraph::sep_adapter {

struct SEPTask {
    TransferEngineType    engine;
    std::vector<uint32_t> partition_indices;
    std::vector<uint32_t> active_vertices;
};

std::vector<SEPTask>
combine_to_sep_tasks(const std::vector<CombinedTask>& combined,
                     const ActivityTracker& tracker);

class SEPWorklistAdapter {
public:
    explicit SEPWorklistAdapter(std::size_t capacity);
    ~SEPWorklistAdapter();
    SEPWorklistAdapter(const SEPWorklistAdapter&) = delete;
    SEPWorklistAdapter& operator=(const SEPWorklistAdapter&) = delete;
    SEPWorklistAdapter(SEPWorklistAdapter&&) noexcept;
    SEPWorklistAdapter& operator=(SEPWorklistAdapter&&) noexcept;
    void load(const std::vector<uint32_t>& vertices);
    std::size_t size() const noexcept;
    void* native_handle() noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace hytgraph::sep_adapter
```

### Bridging logic

- Preserve `CombinedTask::partition_indices` invariant (non-consecutive allowed).
- Load active vertices into `groute::worklist`.
- Reuse one worklist adapter across tasks; reload each time.

### Exit criterion

- Executable task count drops by ≥3× for filter-heavy workloads.
- Results unchanged vs Phase 14.

## Phase 16 — Contribution scheduling bridge

**Goal:** Feed priorities into the engine's ordering.

### New files

- `include/sep_adapter/sep_contribution_scheduler.hpp` + `src/sep_adapter/sep_contribution_scheduler.cpp`
- `include/sep_adapter/sep_priority_buffer.hpp` + `src/sep_adapter/sep_priority_buffer.cpp`

### Public interface

```cpp
namespace hytgraph::sep_adapter {

enum class SchedulingMode { Synchronous, ContributionDriven };

struct CDSConfig {
    SchedulingMode mode = SchedulingMode::Synchronous;
    float priority_floor = 0.0f;
};

class SEPContributionScheduler {
public:
    explicit SEPContributionScheduler(CDSConfig config);
    std::vector<SEPTask>
    order(std::vector<SEPTask> tasks,
          const std::vector<float>& priorities) const;
private:
    CDSConfig config_;
};

} // namespace hytgraph::sep_adapter
```

### Bridging logic

- `Synchronous` mode: `order()` is a no-op.
- `ContributionDriven` mode: reorder by aggregate priority; aggregate by **max**; stable sort with tie-break on first partition index.
- Document the aggregation rule; do not change it silently in later phases.

### Exit criterion

- Iteration count improves by ≥1 on hub-heavy graphs vs synchronous.
- Synchronous mode is bit-identical to Phase 15.

## Phase 17 — VCGC read path

**Goal:** Merge cached subgraph into the engine's `GraphDatum`.

### New files

- `include/sep_adapter/sep_vcgc_integration.hpp` + `src/sep_adapter/sep_vcgc_integration.cpp`
- `include/sep_adapter/sep_cached_graph_view.hpp` + `src/sep_adapter/sep_cached_graph_view.cpp`

### Public interface

```cpp
namespace hytgraph::sep_adapter {

struct VCGCConfig {
    bool        enabled = true;
    std::size_t capacity_bytes = 2ull * 1024 * 1024 * 1024;
};

class SEPVCGCIntegration {
public:
    SEPVCGCIntegration(SEPExecutionDriver& driver, VCGCConfig config);
    SEPExecutionResult prepare_iteration(const VertexCache& cache);
    struct Stats {
        std::uint64_t hits = 0;
        std::uint64_t misses = 0;
        std::uint64_t cached_vertices = 0;
        std::uint64_t cached_bytes = 0;
    };
    Stats last_stats() const noexcept;
private:
    SEPExecutionDriver& driver_;
    VCGCConfig          config_;
    Stats               stats_;
};

} // namespace hytgraph::sep_adapter
```

### Bridging logic

- Build a `GraphDatum` for cached vertices; merge with the normal payload.
- Compute hit/miss at payload-building.
- Enforce VCGC-disabled for BFS in the factory, not at runtime.

### Exit criterion

- Cache-on vs cache-off correctness passes (`1e-6`).
- Hit/miss accounting exact.
- Independently disableable via `VCGCConfig::enabled`.

## Phase 18 — VCGC refresh

**Goal:** Lazy cache refresh with 30% threshold.

### New files

- `include/sep_adapter/sep_vcgc_refresh.hpp` + `src/sep_adapter/sep_vcgc_refresh.cpp`

### Public interface

```cpp
namespace hytgraph::sep_adapter {

struct RefreshConfig {
    float threshold = 0.30f;
    bool  enabled   = true;
};

struct RefreshStats {
    std::uint64_t refresh_count    = 0;
    std::uint64_t evicted_vertices = 0;
    std::uint64_t loaded_vertices  = 0;
    double        refresh_time_ms  = 0.0;
};

class SEPVCGCRefresh {
public:
    SEPVCGCRefresh(SEPVCGCIntegration& integration, RefreshConfig config);
    SEPExecutionResult maybe_refresh(const VertexCache& cache,
                                     const std::vector<uint32_t>& new_candidates);
    RefreshStats stats() const noexcept;
private:
    SEPVCGCIntegration& integration_;
    RefreshConfig       config_;
    RefreshStats        stats_;
};

} // namespace hytgraph::sep_adapter
```

### Bridging logic

- Trigger only when candidate-set difference exceeds 30%.
- Evict cold → compact → allocate new space.
- Fold loading into next `execute_step()`.
- Measure refresh time excluding loading time.

### Exit criterion

- Threshold boundaries respected.
- Overhead measurable and reported separately.

## Phase 19 — Multi-stream runtime

**Goal:** Use `groute::Stream` for overlap. Demonstrate actual overlap via profiling.

### New files

- `include/sep_adapter/sep_stream_manager.hpp` + `src/sep_adapter/sep_stream_manager.cpp`
- `include/sep_adapter/sep_stream_scheduler.hpp` + `src/sep_adapter/sep_stream_scheduler.cpp`

### Public interface

```cpp
namespace hytgraph::sep_adapter {

class SEPStreamManager {
public:
    explicit SEPStreamManager(std::size_t stream_count);
    ~SEPStreamManager();
    void* next_stream();
    void synchronize_all();
    std::size_t stream_count() const noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

struct StreamSchedulingConfig {
    std::size_t stream_count = 4;
    bool paper_priority_order = true;
};

class SEPStreamScheduler {
public:
    explicit SEPStreamScheduler(StreamSchedulingConfig config);
    std::vector<std::vector<SEPTask>>
    schedule(const std::vector<SEPTask>& tasks,
             SEPStreamManager& manager);
private:
    StreamSchedulingConfig config_;
};

} // namespace hytgraph::sep_adapter
```

### Bridging logic

- Use `groute::Stream`, never raw `cudaStream_t`.
- Paper priority order: filter → zero-copy → compaction.
- Verify overlap with `nsys`. Do not claim overlap without profiler evidence.

### Exit criterion

- Correct for 1, 2, 4, 8 streams.
- Profiling shows at least one memcpy overlapping one kernel.

## Phase 20 — Full HyTGraph integration

**Goal:** Assemble the complete pipeline with feature toggles.

### New files

- `include/sep_adapter/full_pipeline.hpp` + `src/sep_adapter/full_pipeline.cpp`
- `include/runtime/pipeline_config.hpp`
- `tests/integration/full_pipeline_test.cpp`
- `docs/full_pipeline.md`

### Public interface

```cpp
namespace hytgraph::runtime {

struct PipelineConfig {
    sep_adapter::Algorithm  algorithm;
    sep_adapter::SEPVariant variant;
    double alpha = 0.80;
    double beta  = 0.40;
    double gamma = 0.625;
    bool        enable_task_combining = true;
    std::size_t task_combine_k        = 4;
    sep_adapter::SchedulingMode scheduling_mode =
        sep_adapter::SchedulingMode::ContributionDriven;
    bool        enable_vcgc            = true;
    std::size_t vcgc_capacity_bytes    = 2ull * 1024 * 1024 * 1024;
    float       vcgc_refresh_threshold = 0.30f;
    std::size_t stream_count = 4;
};

} // namespace hytgraph::runtime
```

```cpp
namespace hytgraph::sep_adapter {

class FullPipeline {
public:
    FullPipeline(const runtime::PipelineConfig& config,
                 const CSRGraph& graph);
    struct RunResult {
        std::uint64_t iterations = 0;
        double total_time_seconds = 0.0;
        double computation_seconds = 0.0;
        double transfer_seconds    = 0.0;
        double compaction_seconds  = 0.0;
        double cache_mgmt_seconds  = 0.0;
    };
    RunResult run(std::uint32_t max_iterations = 1000);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace hytgraph::sep_adapter
```

### Integration notes

- `FullPipeline` owns one instance of each component.
- Every config flag toggles one component on/off.
- Per-component times are measured, never modeled.
- Support `HYTGRAPH_WITH_SEP_GRAPH=OFF` build with the null driver.

### Exit criterion

- End-to-end run for PageRank, SSSP, BFS, CC.
- Every flag independently changes behavior and is measurable.
- Null-driver build produces correct results.

## Phase 21 — Evaluation

**Goal:** Produce paper-style ablation tables and plots.

### New files

- `experiments/configs/ablation.yaml`
- `experiments/configs/baselines.yaml`
- `experiments/configs/sensitivity.yaml`
- `experiments/run_experiment.py`
- `experiments/collect_results.py`
- `experiments/plot_results.py`
- `docs/evaluation_report.md`

### Required experiments

1. Engine-selection behavior across iterations.
2. Overall runtime vs internal baselines.
3. Transfer-volume reduction (Table V style).
4. Paper-style ablation chain.
5. Engineering ablation chain.
6. VCGC vs hub-cache vs no-cache.
7. Cache-size sensitivity.
8. Graph-size sensitivity.
9. Algorithm comparison (PageRank, SSSP, BFS, CC).

### Per-experiment metrics

- Wall-clock (5 runs, median + spread).
- Per-component times (measured only).
- Transfer bytes.
- Active-vertex and active-edge counts.
- Engine-selection counts.
- Cache hits/misses.
- Iteration count to convergence.
- Full YAML config dump.
- Hardware fingerprint.
- Git commit hash.

### Exit criterion

- All figures regenerate from one command.
- Report contains "Paper Fidelity" and "Engineering Approximations" sections.

---

# 8. Data model and partitioning

```cpp
struct CSRGraph {
    uint64_t num_vertices;
    uint64_t num_edges;
    std::vector<uint64_t> row_offsets;
    std::vector<uint32_t> column_indices;
    std::vector<float> edge_weights;
};
```

Rules:

- `row_offsets[v+1] - row_offsets[v]` is out-degree.
- Use 64-bit edge offsets for large graphs.
- Validate monotonic offsets and valid vertex IDs.
- Do not silently reorder vertices unless hub preprocessing is explicitly enabled.
- Logical partition size defaults to 32 MB.
- Execution-task granularity remains separate from logical partition size.

---

# 9. HyTM cost model

Implement the paper's equations exactly before optimizing them. The reference implementation should be CPU-side and unit-tested before a GPU cost evaluator is introduced.

```text
ExpTM-Filter:
T_efi = ceil((sum Do(v)) * d1 / m / MR) * RTT

ExpTM-Compaction:
T_eci = ceil((sum over active v [Do(v)*d1] + |Ai|*d2) / m / MR) * RTT
        + (sum over active v [Do(v)*d1] + |Ai|*d2) / T_hptcpt

ImpTM-Zero-Copy:
T_izi = ceil(sum over active v [ceil(Do(v)*d1/m) + am(v)] / MR) * RTT_zc

RTT_zc = gamma*RTT + (1-gamma)*(active_edges/total_edges)*RTT
```

Paper-aligned parameters:

- `m = 128 bytes`
- `MR = 256`
- `gamma = 0.625` for the zero-copy RTT model
- `alpha = 0.80`
- `beta = 0.40`
- `am(v)` = alignment/request-fragmentation overhead

Selection:

```text
if T_eci < alpha*T_efi and T_eci < beta*T_izi:
    choose ExpTM-Compaction
else if T_efi < T_izi:
    choose ExpTM-Filter
else:
    choose ImpTM-Zero-Copy
```

---

# 10. Task combining

- Keep logical partitions fine-grained for cost analysis.
- Combine up to `k = 4` consecutive filter partitions.
- Combine selected active vertices for compaction into a contiguous task.
- Combine selected zero-copy vertices into a single kernel/task.
- Record logical partition count and executable task count.

---

# 11. Scheduling

## 11.1 Hub sorting

```text
H(v) = Do(v) * Di(v) / (Do_max * Di_max)
```

- Select/group approximately the top 8% important vertices.
- Hub sorting supports both contribution-driven scheduling and grouping of likely-active subgraphs for HyTM engine selection.
- Perform as preprocessing rather than every iteration.

## 11.2 Contribution-driven scheduling

- Prioritize partitions/vertices by contribution or delta.
- Keep synchronous execution as the correctness reference.
- Measure stale/redundant work and scheduling overhead.
- Integrate priorities with the SEP worklist/execution layer after Phase 13.
- Do not assume asynchronous execution is automatically faster.

---

# 12. VCGC

Implement vertex-centric graph caching as described by the paper rather than as a generic cache.

Requirements:

- Maintain a `|V|`-length hotness array.
- Paper implementation uses one byte per vertex.
- Track access frequency.
- Select top-K candidates subject to cache capacity.
- Follow the paper's VCGC execution flow.

## 12.1 Cache refresh

Conceptually:

```text
current cache
    ↓
candidate generation
    ↓
compare candidates/current cache
    ↓
evict cold entries
    ↓
compact surviving cache data
    ↓
allocate new space
    ↓
normal HyTM computation
    ↓
load new candidates during computation where possible
```

Requirements:

- replacement threshold = 30%
- measure cache-management overhead separately
- keep cache-control intervals configurable
- do not automatically equate cache-control gamma with the zero-copy cost-model gamma

---

# 13. Runtime and CUDA execution

- Establish the SEP-backed single-stream execution path before enabling overlap.
- Keep the existing GPU baseline/reference path available for correctness comparison.
- Use `groute::Stream` (from vendored code) for all stream management. Never raw `cudaStream_t`.
- Overlap CPU compaction, host/device transfers, and GPU computation.
- Follow paper-aligned task-priority behavior where practical.
- Use profiling to verify actual overlap.
- Do not claim overlap merely because multiple streams exist.

---

# 14. Baselines

## Internal

```text
Reference
ExpTM-Filter
ExpTM-Compaction
ImpTM-Zero-Copy
HyTM
HyTM + Task Combining
HyTM + Task Combining + Contribution-Driven Scheduling
HyTGraph
```

## Paper comparisons

Where feasible:

```text
Subway
EMOGI
Grus
cuGraph
Galois
ImpTM-UM / Unified Memory
```

Reuse existing external implementations where possible. Record version and configuration.

---

# 15. Required experiments

1. Engine-selection behavior across iterations for PageRank and SSSP.
2. Overall runtime against internal and feasible paper baselines.
3. Transfer-volume and communication-reduction analysis.
4. Paper-style optimization ablation.
5. VCGC vs hub-cache vs no-cache.
6. Cache-size sensitivity.
7. Graph-size sensitivity.
8. Algorithm comparison across PageRank, SSSP, BFS and, if feasible, CC.

## 15.1 Paper-style ablation

```text
ImpTM-Zero-Copy
      ↓
+ HyTM
      ↓
+ VCGC
      ↓
+ Task Combining
      ↓
+ Contribution-Driven Scheduling
```

## 15.2 Engineering ablation

```text
Reference
 → HyTM
 → + Task Combining
 → + Contribution-Driven Scheduling
 → + VCGC
 → Full HyTGraph
```

---

# 16. Dataset strategy

Start small and scale progressively:

1. hand-built graphs;
2. small real graphs;
3. sparse graphs;
4. power-law graphs;
5. large graphs;
6. synthetic PaRMAT/RMAT-style graphs.

For paper-aligned synthetic experiments use:

```text
a = 0.5
b = 0.2
c = 0.2
```

Record:

- dataset name
- source/version
- vertex count
- edge count
- graph type
- preprocessing
- checksum

---

# 17. Metrics

| Area          | Metrics                                                                |
| ------------- | ---------------------------------------------------------------------- |
| Runtime       | total, computation, transfer, compaction, scheduling, cache-management |
| Communication | bytes transferred, active edges, remote accesses, reduction %          |
| HyTM          | filter/compaction/zero-copy selections                                 |
| Scheduling    | logical partitions, executable tasks, overhead, redundant/stale work   |
| VCGC          | hits, misses, hit rate, refresh count, cache size, management time     |
| Correctness   | result comparison, convergence/iteration behavior                      |

---

# 18. Modeled vs measured behavior

Clearly distinguish:

### Measured

- actual CUDA kernel time
- actual CPU compaction time
- actual transfer time
- actual runtime

### Modeled

- PCIe/TLP cost
- zero-copy RTT
- request-fragmentation estimates
- other analytical cost-model values

Do not combine modeled and measured timings without explicit labeling.

If hardware differs from the paper's platform, compare relative behavior first.

---

# 19. Configuration and reproducibility

Example:

```yaml
algorithm: pagerank

partition:
  size_bytes: 33554432

transfer:
  mode: hybrid
  request_size: 128
  max_requests_per_tlp: 256
  alpha: 0.80
  beta: 0.40
  gamma: 0.625

scheduler:
  mode: contribution
  task_combine_k: 4

cache:
  enabled: true
  size_bytes: 2147483648
  refresh_threshold: 0.30

execution:
  streams: 4
```

Rules:

- No important experimental parameter should be hard-coded.
- Record configuration.
- Record git commit.
- Record hardware.
- Record software versions.
- Record timestamp.
- Use 5 runs for final comparisons where practical.

---

# 20. Testing strategy

## Unit tests

Test:

- CSR invariants and graph loading
- partition boundaries and edge counts
- active-edge calculations
- cost-model equations
- engine-selection branches
- SEP variant mapping (project enum ↔ `sepgraph::common::AlgoVariant`)
- SEP adapter lifecycle (init, single step, invalid config)
- task combination
- hub scoring and ordering
- hotness updates
- candidate selection
- cache insertion/eviction/refresh
- worklist adapter load/size round-trips

## Integration tests

Test:

- CPU vs GPU PageRank
- CPU vs GPU SSSP
- equivalent results across transfer engines
- equivalent results across SEP execution variants where applicable
- HyTM selection on known activity patterns
- cache-enabled vs cache-disabled correctness
- synchronous vs asynchronous correctness
- SEP-backed path vs the legacy GPU baseline path
- multi-stream vs single-stream correctness

---

# 21. Acceptance criteria

| Category           | Acceptance                                                                                           |
| ------------------ | ---------------------------------------------------------------------------------------------------- |
| Correctness        | Implemented algorithms agree with reference within documented tolerance.                             |
| SEP execution      | SEP-backed PageRank/SSSP execution is correct and independently testable against the reference path. |
| HyTM               | All three transfer mechanisms work and selector decisions are testable.                              |
| Task combining     | Task count decreases without changing logical partition decisions.                                   |
| CDS                | Contribution prioritization works without breaking correctness.                                      |
| VCGC               | Cache hit/miss, candidate selection and refresh are correct.                                         |
| Behavior           | Engine selection and communication trends qualitatively match the paper where expected.              |
| Evaluation         | Major paper evaluation questions can be reproduced.                                                  |
| Performance        | Project approaches the 60–70% target where hardware allows meaningful comparison.                    |
| Scientific honesty | Every deviation/approximation is documented.                                                         |

---

# 22. Milestones

| Milestone | Deliverable                           | Status      |
| --------- | ------------------------------------- | ----------- |
| M0        | Repository/build/test infrastructure  | NOT STARTED |
| M1        | CSR                                   | NOT STARTED |
| M2        | CPU references                        | NOT STARTED |
| M3        | GPU baselines                         | NOT STARTED |
| M4        | Activity tracking                     | NOT STARTED |
| M5        | Partitioning                          | NOT STARTED |
| M6        | ExpTM-Filter                          | NOT STARTED |
| M7        | ExpTM-Compaction                      | NOT STARTED |
| M8        | ImpTM-Zero-Copy                       | NOT STARTED |
| M9        | HyTM                                  | NOT STARTED |
| M10       | Task Combining                        | NOT STARTED |
| M11       | Hub Sorting                           | NOT STARTED |
| M12       | Vendored HyTGraph SEP-Graph organized | NOT STARTED |
| M13       | SEP execution driver adapter          | NOT STARTED |
| M14       | HyTGraph ↔ SEP data-movement bridge   | NOT STARTED |
| M15       | Task Combining with SEP worklists     | NOT STARTED |
| M16       | Contribution scheduling with SEP      | NOT STARTED |
| M17       | VCGC on SEP                           | NOT STARTED |
| M18       | VCGC refresh on SEP                   | NOT STARTED |
| M19       | Multi-stream runtime with SEP         | NOT STARTED |
| M20       | Full HyTGraph integration             | NOT STARTED |
| M21       | Evaluation                            | NOT STARTED |

---

# 23. Persistent project state

Update this section after every coding chat.

```text
CURRENT PHASE:
CURRENT MILESTONE:
CURRENT TASK:
STATUS:

IMPLEMENTED:
- ...

FILES CHANGED:
- ...

INTERFACES CHANGED:
- ...

TESTS:
- ...

MEASUREMENTS:
- ...

PAPER FEATURES:
- [ ] CSR
- [ ] Partitioning
- [ ] Vendored HyTGraph SEP-Graph
- [ ] SEP execution driver adapter
- [ ] SEP-backed GPU execution
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

ALGORITHMS:
- [ ] PageRank
- [ ] SSSP
- [ ] BFS
- [ ] CC

DATASETS:
- ...

KNOWN ISSUES:
- ...

PAPER FIDELITY:
Paper-derived:
- ...

Engineering approximations:
- ...

Not implemented:
- ...

NEXT TASK:
- ...
```

---

# 24. Standard prompt for every new coding chat

```text
We are building a reproduction of the attached HyTGraph paper.

I am providing:
1. the original HyTGraph paper;
2. MASTER_PLAN.md;
3. the current project/codebase;
4. the latest project-state handoff.

Read the paper and master plan before coding.

SOURCE RULE:
- The paper is authoritative for what HyTGraph does.
- MASTER_PLAN.md is authoritative for how our project is organized.
- The codebase is authoritative for what currently exists.
- This chat's task is authoritative for what you should change now.

CURRENT TASK:
[EXACT TASK]

Before coding:
1. Explain the relevant paper mechanism.
2. Explain how it fits our architecture.
3. Identify relevant paper dependencies such as SEP-Graph, Groute, Subway, CUB, or CUDA streams.
4. Identify any approximation.
5. List files you expect to modify.

Then implement ONLY this task.
Add appropriate tests.

At the end provide:
FILES CHANGED:
TESTS:
PAPER FIDELITY:
APPROXIMATIONS:
KNOWN ISSUES:
NEXT TASK:
PROJECT STATE UPDATE:
```

---

# 25. Current coding task

Start with **Phase 12** (organize the working HyTGraph SEP-Graph code). Do not implement the adapter yet.

1. Write `docs/original_hytgraph_build_notes.md` capturing the exact working configuration (CUDA version, GPU, CMake, GCC, every fix applied, exact build command).
2. Run the original repo's samples (`hybrid_pr`, `hybrid_sssp`, `hybrid_bfs`, `hybrid_cc`) on a small graph; record outputs in `docs/original_hytgraph_reference_runs.md`.
3. Copy `include/framework/`, `include/groute/`, `include/utils/`, and `deps/` into `third_party/hytgraph_sep/`; apply the same fixes.
4. Create `third_party/hytgraph_sep/CMakeLists.txt` producing `hytgraph_sep_lib`.
5. Add `HYTGRAPH_WITH_SEP_GRAPH` option and `add_subdirectory` in root `CMakeLists.txt`.
6. Add `tests/integration/sep_graph_link_smoke_test.cpp`.
7. Verify no `sepgraph::` or `groute::` symbol leaks outside `third_party/hytgraph_sep/` and the smoke test.
8. Confirm all Phase 0–11 tests still pass unchanged.
9. End with a project-state handoff.

Do **not** add `sep_adapter` in this phase. Do **not** modify the copied vendored source.

**Time-box: 2 days.**

---

# 26. Final development principle

```text
ORIGINAL PAPER
      +
MASTER PLAN
      +
CURRENT CODEBASE
      +
CURRENT PROJECT STATE
      +
ONE BOUNDED TASK
      =
SAFE, ITERATIVE LLM-ASSISTED IMPLEMENTATION
```

**Do not turn the master plan into a second copy of the paper.**

Keep paper details in the paper. Keep project architecture, sequencing, decisions, approximations, acceptance criteria, and state in this document.

**END OF MASTER IMPLEMENTATION PLAN**
