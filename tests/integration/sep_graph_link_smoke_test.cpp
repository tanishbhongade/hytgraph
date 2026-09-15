// tests/integration/sep_graph_link_smoke_test.cpp
//
// Phase 12 exit criterion:
//   Prove that the main project can link the vendored HyTGraph SEP-Graph
//   headers through the hytgraph_sep_lib target, without any data
//   movement, kernel launch, or engine instantiation.
//
// Per MASTER_PLAN.md §Phase 12 step 5:
//   - Include ONE vendored header.
//   - Instantiate a sepgraph::common::AlgoVariant value.
//   - Do not launch kernels.
//
// DEVIATIONS FROM THE MASTER PLAN (all forced by the vendored code):
//
//   (a) The plan's example header, framework/algo_variants.cuh, is not
//       standalone-includable: it pulls in framework/hybrid_policy.h,
//       which depends on app-side definitions not present in a bare
//       smoke-test translation unit. We include framework/common.h
//       directly, which is where AlgoVariant is actually declared.
//
//   (b) AlgoVariant is a CLASS, not an enum. It has a default constructor
//       and a 3-argument constructor (Model, MsgPassing, Scheduling).
//       There is no integer representation to round-trip. We default-
//       construct and copy-construct.
//
//   (c) The file is .cpp and is compiled as PLAIN C++ (no nvcc).
//       framework/common.h is pure C++.
//
// The shim block below supplies standard-library headers that the
// vendored headers use without including themselves. This is a
// build-environment shim, not a modification of the vendored tree:
//
//   <tuple>   — common.h uses std::tie without including <tuple>
//   <string>  — common.h uses std::string without including <string>
//
// This is the ONLY project file allowed to include a vendored header
// outside of src/sep_adapter/ (see MASTER_PLAN.md §6 integration rule).

#include <cstdint>
#include <cstdio>
#include <string>
#include <tuple>

#include <framework/common.h>

int main()
{
    using sepgraph::common::AlgoVariant;

    // Default-construct. Proves the type is complete and the default
    // constructor is visible.
    AlgoVariant v;

    // Copy-construct. Proves the type is not abstract.
    AlgoVariant copy = v;
    (void)copy;

    static_assert(sizeof(AlgoVariant) > 0,
                  "AlgoVariant must be a complete type");

    std::printf("sep_graph_link_smoke_test: OK (AlgoVariant size=%zu)\n",
                sizeof(AlgoVariant));
    return 0;
}