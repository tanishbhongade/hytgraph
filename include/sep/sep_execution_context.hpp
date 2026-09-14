#ifndef HYTGRAPH_SEP_EXECUTION_CONTEXT_HPP
#define HYTGRAPH_SEP_EXECUTION_CONTEXT_HPP

#include <cstddef>

#include "sep_execution_config.hpp"
#include "sep_frontier.hpp"

namespace hytgraph::sep
{

    /**
     * Non-owning execution context for the SEP foundation.
     *
     * This type intentionally contains only references/pointers to state that is
     * owned elsewhere by the existing project architecture.
     *
     * SEP-Graph's concrete implementation packages graph and execution state in
     * GraphDatum. Our project already separates graph data, activity tracking,
     * partitions, and tasks, so Phase 12 does NOT reproduce GraphDatum as a
     * replacement object.
     *
     * Instead, this context defines the integration boundary that a future
     * execution driver can consume.
     *
     * Ownership:
     *   None of the pointers below are owned by this class.
     *
     * Lifetime:
     *   All referenced objects must outlive the execution context.
     *
     * Phase 12:
     *   This is an interface/boundary only.
     *
     * Phase 13:
     *   Concrete graph/activity/partition/task types can be bound here.
     *
     * Phase 14:
     *   HyTGraph transfer state must remain outside this context unless an
     *   explicit integration layer requires it.
     */
    class SEPExecutionContext
    {
    public:
        /**
         * Opaque handles are used deliberately in Phase 12.
         *
         * We do not know yet that the existing concrete graph/activity/task
         * classes should depend on SEP headers. More importantly, SEP should not
         * force those classes to inherit from SEP types.
         *
         * Later phases can replace these opaque handles with narrowly scoped
         * typed adapters once the actual CUDA execution boundary is implemented.
         */
        using graph_handle_type = void;
        using activity_handle_type = void;
        using partition_handle_type = void;
        using task_handle_type = void;

        SEPExecutionContext() noexcept = default;

        /**
         * Construct a context from existing project-owned state.
         *
         * No object is copied or transferred.
         */
        SEPExecutionContext(
            graph_handle_type *graph,
            activity_handle_type *activity = nullptr,
            partition_handle_type *partitions = nullptr,
            task_handle_type *tasks = nullptr,
            SEPFrontier *frontier = nullptr) noexcept
            : graph_(graph),
              activity_(activity),
              partitions_(partitions),
              tasks_(tasks),
              frontier_(frontier) {}

        graph_handle_type *graph() noexcept
        {
            return graph_;
        }

        const graph_handle_type *graph() const noexcept
        {
            return graph_;
        }

        activity_handle_type *activity() noexcept
        {
            return activity_;
        }

        const activity_handle_type *activity() const noexcept
        {
            return activity_;
        }

        partition_handle_type *partitions() noexcept
        {
            return partitions_;
        }

        const partition_handle_type *partitions() const noexcept
        {
            return partitions_;
        }

        task_handle_type *tasks() noexcept
        {
            return tasks_;
        }

        const task_handle_type *tasks() const noexcept
        {
            return tasks_;
        }

        SEPFrontier *frontier() noexcept
        {
            return frontier_;
        }

        const SEPFrontier *frontier() const noexcept
        {
            return frontier_;
        }

        /**
         * Whether graph state has been attached.
         */
        bool has_graph() const noexcept
        {
            return graph_ != nullptr;
        }

        /**
         * Whether an activity representation has been attached.
         */
        bool has_activity() const noexcept
        {
            return activity_ != nullptr;
        }

        /**
         * Whether partition state has been attached.
         */
        bool has_partitions() const noexcept
        {
            return partitions_ != nullptr;
        }

        /**
         * Whether task state has been attached.
         */
        bool has_tasks() const noexcept
        {
            return tasks_ != nullptr;
        }

        /**
         * Whether a DD frontier has been attached.
         */
        bool has_frontier() const noexcept
        {
            return frontier_ != nullptr;
        }

        /**
         * A DD execution requires a frontier.
         *
         * TD execution does not require one because topology traversal is not
         * selected through the active frontier.
         */
        bool satisfies_variant_requirements(
            const SEPExecutionVariant &variant) const noexcept
        {
            if (variant.is_data_driven())
            {
                return frontier_ != nullptr;
            }

            return true;
        }

    private:
        graph_handle_type *graph_{nullptr};
        activity_handle_type *activity_{nullptr};
        partition_handle_type *partitions_{nullptr};
        task_handle_type *tasks_{nullptr};
        SEPFrontier *frontier_{nullptr};
    };

} // namespace hytgraph::sep

#endif // HYTGRAPH_SEP_EXECUTION_CONTEXT_HPP