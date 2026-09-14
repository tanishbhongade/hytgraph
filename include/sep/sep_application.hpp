#ifndef HYTGRAPH_SEP_APPLICATION_HPP
#define HYTGRAPH_SEP_APPLICATION_HPP

#include <cstddef>
#include <limits>

#include "sep_execution_variant.hpp"

namespace hytgraph::sep
{

    /**
     * Application-facing SEP execution contract.
     *
     * This interface captures the semantic operations required by the SEP
     * execution model without implementing GPU execution.
     *
     * The contract is intentionally independent of the project's concrete
     * CSRGraph, ActivityTracker, Partition, Task, and worklist classes.
     *
     * Phase 12:
     *   - establishes the algorithm/execution boundary;
     *   - allows PageRank and SSSP adapters to describe their SEP semantics;
     *   - does not execute kernels.
     *
     * Phase 13:
     *   - a CUDA/device adapter can map these operations onto the SEP-derived
     *     execution variants.
     *
     * The operation set is adapted from SEP-Graph's AppBase:
     *   GetInitValue
     *   GetInitBuffer
     *   GetIdentityElement
     *   CombineValueBuffer
     *   AccumulateBuffer
     *   IsActiveNode
     *   PostComputation
     *   IsHighPriority
     *
     * The original SEP implementation exposes these as CUDA/device-oriented
     * hooks. This project-local Phase 12 interface keeps the same semantic
     * boundary while avoiding a dependency on SEP-Graph's legacy CUDA/Groute
     * GraphDatum types.
     */
    template <typename TValue,
              typename TBuffer,
              typename TWeight = double>
    class SEPApplication
    {
    public:
        using value_type = TValue;
        using buffer_type = TBuffer;
        using weight_type = TWeight;
        using node_id_type = std::size_t;

        /**
         * Result of combining a vertex's current value with its received buffer.
         *
         * new_buffer:
         *     message/update that may be propagated by a subsequent execution
         *     operation.
         *
         * activate:
         *     whether the vertex should become part of the next active work set.
         */
        struct CombineResult
        {
            TBuffer new_buffer{};
            bool activate{false};
        };

        /**
         * Result of accumulating one message into a destination buffer.
         *
         * changed:
         *     whether the destination buffer changed.
         *
         * activate:
         *     whether the destination should become active.
         */
        struct AccumulateResult
        {
            bool changed{false};
            bool activate{false};
        };

        virtual ~SEPApplication() = default;

        /**
         * Configure the application for the selected SEP execution variant.
         *
         * This is intentionally a configuration operation only. It does not
         * allocate graph storage or launch GPU work.
         */
        virtual void set_execution_variant(
            const SEPExecutionVariant &variant) = 0;

        /**
         * Return the currently configured execution variant.
         */
        virtual const SEPExecutionVariant &execution_variant() const noexcept = 0;

        /**
         * Initialize the persistent value associated with one vertex.
         *
         * Corresponds to SEP-Graph AppBase::GetInitValue().
         */
        virtual TValue init_value(node_id_type node) const = 0;

        /**
         * Initialize the transient/received buffer associated with one vertex.
         *
         * Corresponds to SEP-Graph AppBase::GetInitBuffer().
         */
        virtual TBuffer init_buffer(node_id_type node) const = 0;

        /**
         * Return the identity element used when accumulating messages.
         *
         * Corresponds to SEP-Graph AppBase::GetIdentityElement().
         */
        virtual TBuffer identity_element() const = 0;

        /**
         * Combine the current vertex value and its accumulated buffer.
         *
         * This is the central application operation used by SEP's execution
         * variants. The concrete interpretation depends on the algorithm.
         *
         * PageRank:
         *     consume accumulated delta and produce a new outgoing delta.
         *
         * SSSP:
         *     consume the best received tentative distance and produce the
         *     corresponding propagation value.
         *
         * The actual GPU implementation is deferred to Phase 13.
         */
        virtual CombineResult combine_value_buffer(
            node_id_type node,
            const TValue &value,
            const TBuffer &buffer) const = 0;

        /**
         * Accumulate a message into a destination vertex's buffer.
         *
         * This overload represents an unweighted graph edge.
         *
         * Corresponds to SEP-Graph's AccumulateBuffer(src, dst, buffer, ...)
         * family.
         */
        virtual AccumulateResult accumulate_buffer(
            node_id_type src,
            node_id_type dst,
            TBuffer &destination,
            const TBuffer &message) const = 0;

        /**
         * Accumulate a weighted message.
         *
         * SSSP requires edge weights, while PageRank can use the unweighted
         * operation above.
         *
         * The default implementation deliberately fails rather than silently
         * ignoring the supplied weight.
         */
        virtual AccumulateResult accumulate_buffer(
            node_id_type src,
            node_id_type dst,
            TWeight weight,
            TBuffer &destination,
            const TBuffer &message) const
        {
            (void)src;
            (void)dst;
            (void)weight;
            (void)destination;
            (void)message;

            return AccumulateResult{};
        }

        /**
         * Determine whether a vertex should be processed by a topology-driven
         * execution path.
         *
         * SEP-Graph invokes this concept for TD variants.
         *
         * For DD variants, the worklist/frontier determines the candidate work,
         * so the execution driver need not use this predicate to enumerate the
         * entire graph.
         */
        virtual bool is_active(
            node_id_type node,
            const TBuffer &buffer) const = 0;

        /**
         * Optional application-level hook after an execution round.
         *
         * The default is intentionally a no-op.
         */
        virtual void post_computation() {}

        /**
         * Determine whether a message/value should be considered high priority.
         *
         * SEP-Graph exposes this hook for priority-based execution. Phase 12
         * establishes the interface only; priority scheduling itself remains
         * outside this foundation unless an existing project scheduler invokes
         * it explicitly.
         */
        virtual bool is_high_priority(
            const TBuffer &current_priority,
            const TBuffer &buffer) const
        {
            (void)current_priority;
            (void)buffer;
            return true;
        }

        /**
         * Whether the application requires edge weights.
         *
         * This allows a future execution driver to distinguish PageRank-like
         * unweighted accumulation from SSSP-like weighted accumulation without
         * inspecting concrete application types.
         */
        virtual bool requires_edge_weights() const noexcept
        {
            return false;
        }

        /**
         * Whether the application has a meaningful active-node predicate for
         * topology-driven execution.
         *
         * The default is true because SEP's TD execution model is explicitly
         * application-aware through IsActiveNode().
         */
        virtual bool supports_activity_predicate() const noexcept
        {
            return true;
        }
    };

} // namespace hytgraph::sep

#endif // HYTGRAPH_SEP_APPLICATION_HPP