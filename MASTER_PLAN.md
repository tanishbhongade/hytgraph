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
GPU computation
   ↓
Vertex-Centric Graph Caching
   ↓
Next iteration
```

---

# 4. Important paper implementation dependencies

These must not be forgotten.

| Dependency | Requirement |
|---|---|
| **SEP-Graph** | HyTGraph uses SEP-Graph's processing kernel as the GPU computation reference; neighbor shifting is enabled for ExpTM-Filter and ExpTM-Compaction. |
| **Subway** | CPU active-edge compaction follows the Subway design and regenerates a compressed neighbor/index representation. |
| **CUB** | Paper implementation uses CUB for sorting, TopK, and compaction where applicable. |
| **CUDA streams** | Multiple streams overlap GPU computation, transfers, and CPU compaction. |
| **Neighbor shifting** | Must be considered when implementing the explicit-transfer engines. |

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
│   └── runtime/{execution_context.hpp,metrics.hpp,config.hpp}
├── src/
├── cuda/
├── tests/
├── experiments/{configs,run_experiment.py,collect_results.py,plot_results.py}
├── datasets/
├── results/
└── docs/
```

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

## Phase 12 — VCGC

Implement:

- one-byte hotness target
- top-K under cache capacity
- cache hit/miss accounting

**Exit criterion:** cache correctness and candidate selection pass.

## Phase 13 — VCGC refresh

Implement:

- candidate generation
- replacement threshold = 30%
- eviction/compaction
- loading new candidates during normal computation where possible

**Exit criterion:** refresh is correct and overhead is measured.

## Phase 14 — Multi-stream runtime

Implement:

- overlap transfers, CPU compaction, and GPU computation
- paper-aligned task priority
- profiling of actual overlap

**Exit criterion:** asynchronous execution is correct and measurable.

## Phase 15 — Full HyTGraph

Integrate:

- HyTM
- Task Combining
- Contribution-Driven Scheduling
- VCGC
- multi-stream runtime

Preserve independent feature switches.

**Exit criterion:** full system runs end-to-end.

## Phase 16 — Evaluation

Implement:

- internal ablations
- paper baselines where feasible
- transfer analysis
- cache analysis
- sensitivity experiments
- final plots

**Exit criterion:** reproducible result directory and report-ready tables/figures.

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

- Use multiple CUDA streams after single-stream correctness is established.
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

| Area | Metrics |
|---|---|
| Runtime | total, computation, transfer, compaction, scheduling, cache-management |
| Communication | bytes transferred, active edges, remote accesses, reduction % |
| HyTM | filter/compaction/zero-copy selections |
| Scheduling | logical partitions, executable tasks, overhead, redundant/stale work |
| VCGC | hits, misses, hit rate, refresh count, cache size, management time |
| Correctness | result comparison, convergence/iteration behavior |

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
- task combination
- hub scoring and ordering
- hotness updates
- candidate selection
- cache insertion/eviction/refresh

## Integration tests

Test:

- CPU vs GPU PageRank
- CPU vs GPU SSSP
- equivalent results across transfer engines
- HyTM selection on known activity patterns
- cache-enabled vs cache-disabled correctness
- synchronous vs asynchronous correctness

---

# 21. Acceptance criteria

| Category | Acceptance |
|---|---|
| Correctness | Implemented algorithms agree with reference within documented tolerance. |
| HyTM | All three transfer mechanisms work and selector decisions are testable. |
| Task combining | Task count decreases without changing logical partition decisions. |
| CDS | Contribution prioritization works without breaking correctness. |
| VCGC | Cache hit/miss, candidate selection and refresh are correct. |
| Behavior | Engine selection and communication trends qualitatively match the paper where expected. |
| Evaluation | Major paper evaluation questions can be reproduced. |
| Performance | Project approaches the 60–70% target where hardware allows meaningful comparison. |
| Scientific honesty | Every deviation/approximation is documented. |

---

# 22. Milestones

| Milestone | Deliverable | Status |
|---|---|---|
| M0 | Repository/build/test infrastructure | NOT STARTED |
| M1 | CSR | NOT STARTED |
| M2 | CPU references | NOT STARTED |
| M3 | GPU baselines | NOT STARTED |
| M4 | Activity tracking | NOT STARTED |
| M5 | Partitioning | NOT STARTED |
| M6 | ExpTM-Filter | NOT STARTED |
| M7 | ExpTM-Compaction | NOT STARTED |
| M8 | ImpTM-Zero-Copy | NOT STARTED |
| M9 | HyTM | NOT STARTED |
| M10 | Task Combining | NOT STARTED |
| M11 | Hub Sorting | NOT STARTED |
| M12 | Contribution Scheduling | NOT STARTED |
| M13 | VCGC | NOT STARTED |
| M14 | Multi-stream runtime | NOT STARTED |
| M15 | Baselines | NOT STARTED |
| M16 | Ablations | NOT STARTED |
| M17 | Final evaluation | NOT STARTED |
| M18 | Report | NOT STARTED |

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
3. Identify relevant paper dependencies such as SEP-Graph, Subway, CUB, or CUDA streams.
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

# 25. First coding task

Start with **Phase 0**. Do not implement HyTGraph itself yet.

1. Create repository and CMake structure.
2. Create CUDA build target.
3. Create test infrastructure.
4. Create configuration handling.
5. Create experiment runner skeleton.
6. Create result schema.
7. Create README/build/test commands.
8. End with a project-state handoff.

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
