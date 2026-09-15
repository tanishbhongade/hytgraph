// src/sep_adapter/apps/pagerank_app.hpp
//
// Phase 14 — Data-movement bridge.
//
// Project-owned PageRank application, derived from the vendored
// sepgraph::api::AppBase. Behaviour is a faithful port of the
// original HyTGraph samples/hybrid_pr/hybrid_pr.cu PageRank struct.
//
// PLACEMENT: PRIVATE header (src/sep_adapter/apps/).
//
//   This file derives from sepgraph::api::AppBase, which is a
//   vendored type with a full definition (not forward-declarable).
//   Including it under include/sep_adapter/ would leak vendored
//   symbols into a public project header, violating the boundary
//   rule (§Phase 12 exit criterion, PROJECT_STATE.md Known Issue
//   #24). Placement under src/ keeps the leak confined to the
//   hytgraph_runtime private include path.
//
// COMPILATION: This header contains __device__ code and MUST be
//   included from a CUDA-enabled translation unit. The only
//   consumer in Phase 14 is src/sep_adapter/sep_engine_adapter_impl.cu.
//
// CONSTANTS: kPageRankAlpha and kPageRankEpsilon are inlined here
//   rather than imported from the original samples/hybrid_pr/
//   hybrid_pr_common.h. Verify against your paper's PageRank
//   definition if a numerical mismatch appears in tests.

#pragma once

#include <framework/variants/api.cuh>

namespace hytgraph
{
    namespace sep_adapter
    {
        namespace apps
        {

            // PageRank damping factor. The standard literature value is 0.85.
            // Note: this is NOT the HyTM cost-model alpha (0.80). They are
            // different symbols.
            constexpr float kPageRankAlpha = 0.85f;

            // Buffer threshold below which a vertex is not scheduled. Matches
            // the original HyTGraph PageRank ("if (buffer > 0.01)").
            constexpr float kPageRankEpsilon = 0.01f;

            template <typename TValue, typename TBuffer, typename TWeight, typename... UnusedData>
            struct PageRank : sepgraph::api::AppBase<TValue, TBuffer, TWeight>
            {
                using Base = sepgraph::api::AppBase<TValue, TBuffer, TWeight>;
                using Base::AccumulateBuffer; // unhide the weighted overload

                // Constructor parameter `error` is supplied by the engine via
                // the `UnusedData...` template pack in InitGraph(...).
                // The member is required to defeat a known nvcc ICE (see the
                // original hybrid_pr.cu comment).
                double m_error;

                explicit PageRank(double error) : m_error(error) {}

                __forceinline__ __device__
                    TValue
                    GetInitValue(index_t /*node*/) const override
                {
                    return static_cast<TValue>(0);
                }

                __forceinline__ __device__
                    TBuffer
                    GetInitBuffer(index_t /*node*/) const override
                {
                    return static_cast<TBuffer>(1.0f - kPageRankAlpha);
                }

                __forceinline__ __host__ __device__
                    TBuffer
                    GetIdentityElement() const override
                {
                    return static_cast<TBuffer>(0);
                }

                __forceinline__ __device__
                    utils::pair<TBuffer, bool>
                    CombineValueBuffer(index_t node,
                                       TValue *p_value,
                                       TBuffer *p_buffer) override
                {
                    TBuffer buffer = atomicExch(p_buffer, static_cast<TBuffer>(0));
                    bool schedule = false;

                    if (buffer > static_cast<TBuffer>(kPageRankEpsilon))
                    {
                        schedule = true;
                        *p_value += static_cast<TValue>(buffer);

                        const int out_degree = static_cast<int>(
                            this->m_csr_graph.end_edge(node) -
                            this->m_csr_graph.begin_edge(node));

                        if (out_degree > 0)
                        {
                            buffer = static_cast<TBuffer>(
                                kPageRankAlpha * static_cast<float>(buffer) /
                                static_cast<float>(out_degree));
                        }
                        else
                        {
                            buffer = static_cast<TBuffer>(0);
                        }
                    }

                    return utils::pair<TBuffer, bool>(buffer, schedule);
                }

                __forceinline__ __device__ int AccumulateBuffer(index_t /*src*/,
                                                                index_t /*dst*/,
                                                                TBuffer *p_buffer,
                                                                TBuffer buffer) override
                {
                    atomicAdd(p_buffer, buffer);
                    return 0;
                }

                __forceinline__ __device__ bool IsActiveNode(index_t /*node*/,
                                                             TBuffer buffer,
                                                             TValue /*value*/) const override
                {
                    return buffer > static_cast<TBuffer>(m_error);
                }

                __forceinline__ __device__
                    TValue
                    sum_value(index_t /*node*/,
                              TValue /*value*/,
                              TBuffer buffer) const override
                {
                    return static_cast<TValue>(buffer);
                }

                __forceinline__ __device__ bool IsHighPriority(TBuffer current_priority,
                                                               TBuffer buffer) const override
                {
                    return current_priority <= buffer;
                }
            };

        } // namespace apps
    } // namespace sep_adapter
} // namespace hytgraph