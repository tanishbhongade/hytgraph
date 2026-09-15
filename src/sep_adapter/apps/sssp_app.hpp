// src/sep_adapter/apps/sssp_app.hpp
//
// Phase 14 — Data-movement bridge.
//
// Project-owned SSSP application, derived from the vendored
// sepgraph::api::AppBase. Behaviour is a faithful port of the
// original HyTGraph samples/hybrid_sssp/hybrid_sssp.cu SSSP struct.
//
// PLACEMENT: PRIVATE header (src/sep_adapter/apps/).
//   See pagerank_app.hpp for the rationale (vendored AppBase
//   derivation; boundary preservation; CUDA-only TU).
//
// COMPILATION: Contains __device__ code; must be included from a
//   CUDA-enabled translation unit (sep_engine_adapter_impl.cu).

#pragma once

#include <framework/variants/api.cuh>

#include <cstdint>
#include <limits>

namespace hytgraph
{
    namespace sep_adapter
    {
        namespace apps
        {

            // Identity element for distance-min SSSP. Matches the original
            // hybrid_sssp_common.h IDENTITY_ELEMENT used by HyTGraph.
            // Must satisfy: min(x, IDENTITY) == x for every valid distance x.
            constexpr std::uint32_t kSSSPIdentityElement =
                std::numeric_limits<std::uint32_t>::max();

            template <typename TValue, typename TBuffer, typename TWeight, typename... UnusedData>
            struct SSSP : sepgraph::api::AppBase<TValue, TBuffer, TWeight>
            {
                using Base = sepgraph::api::AppBase<TValue, TBuffer, TWeight>;
                using Base::AccumulateBuffer; // unhide the weighted overload

                // Source vertex, supplied by the engine via the UnusedData...
                // template pack in InitGraph(source_node).
                index_t m_source_node;

                explicit SSSP(index_t source_node) : m_source_node(source_node) {}

                __forceinline__ __device__
                    TValue
                    GetInitValue(index_t /*node*/) const override
                {
                    return static_cast<TValue>(kSSSPIdentityElement);
                }

                __forceinline__ __device__
                    TBuffer
                    GetInitBuffer(index_t node) const override
                {
                    if (node == m_source_node)
                    {
                        return static_cast<TBuffer>(0);
                    }
                    return static_cast<TBuffer>(kSSSPIdentityElement);
                }

                __forceinline__ __host__ __device__
                    TBuffer
                    GetIdentityElement() const override
                {
                    return static_cast<TBuffer>(kSSSPIdentityElement);
                }

                __forceinline__ __device__
                    utils::pair<TBuffer, bool>
                    CombineValueBuffer(index_t /*node*/,
                                       TValue *p_value,
                                       TBuffer *p_buffer) override
                {
                    // Note: the original HyTGraph SSSP reads *p_buffer without
                    // atomic exchange (unlike PageRank). Preserved here verbatim.
                    const TBuffer buffer = *p_buffer;
                    bool schedule = false;

                    if (*p_value > buffer)
                    {
                        *p_value = buffer;
                        schedule = true;
                    }

                    return utils::pair<TBuffer, bool>(buffer, schedule);
                }

                __forceinline__ __device__ int AccumulateBuffer(index_t /*src*/,
                                                                index_t /*dst*/,
                                                                TWeight weight,
                                                                TBuffer *p_buffer,
                                                                TBuffer buffer) override
                {
                    atomicMin(p_buffer, static_cast<TBuffer>(buffer + weight));
                    return 1; // ACCUMULATE_SUCCESS_CONTINUE
                }

                __forceinline__ __device__ bool IsActiveNode(index_t /*node*/,
                                                             TBuffer buffer,
                                                             TValue value) const override
                {
                    return buffer < value;
                }

                __forceinline__ __device__
                    TValue
                    sum_value(index_t /*node*/,
                              TValue value,
                              TBuffer buffer) const override
                {
                    if (value > buffer * 2)
                    {
                        return static_cast<TValue>(2);
                    }
                    return static_cast<TValue>(1);
                }

                __forceinline__ __device__ bool IsHighPriority(TBuffer current_priority,
                                                               TBuffer buffer) const override
                {
                    return current_priority > buffer;
                }
            };

        } // namespace apps
    } // namespace sep_adapter
} // namespace hytgraph