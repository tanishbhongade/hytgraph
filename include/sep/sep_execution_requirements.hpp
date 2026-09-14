#ifndef HYTGRAPH_SEP_EXECUTION_REQUIREMENTS_HPP
#define HYTGRAPH_SEP_EXECUTION_REQUIREMENTS_HPP

#include "sep_execution_config.hpp"
#include "sep_execution_context.hpp"

namespace hytgraph::sep
{

    /**
     * Validate whether an execution configuration can be used with a given
     * execution context.
     *
     * Phase 12 validation is intentionally structural:
     *
     *   - a graph must be present;
     *   - DATA_DRIVEN variants require a frontier;
     *   - TOPOLOGY_DRIVEN variants do not require a frontier.
     *
     * This function does not validate:
     *
     *   - CUDA availability;
     *   - graph correctness;
     *   - partition contents;
     *   - algorithm convergence;
     *   - transfer state;
     *   - GPU memory;
     *   - runtime performance.
     *
     * Those belong to later execution/integration phases.
     */
    inline bool is_valid_execution_configuration(
        const SEPExecutionConfig &config,
        const SEPExecutionContext &context) noexcept
    {
        if (!context.has_graph())
        {
            return false;
        }

        return context.satisfies_variant_requirements(config.variant());
    }

    /**
     * Return whether a variant is structurally valid.
     *
     * All combinations represented by SEPExecutionVariant are valid in the
     * Phase 12 execution model.
     *
     * The Cartesian product is:
     *
     *   2 execution modes
     *   × 2 message-passing modes
     *   × 2 scheduling modes
     *   = 8 variants
     *
     * This function exists so future selection code has an explicit validation
     * boundary rather than relying on assumptions about enum values.
     */
    constexpr bool is_valid_variant(
        const SEPExecutionVariant &variant) noexcept
    {
        switch (variant.execution_mode())
        {
        case ExecutionMode::SYNC:
        case ExecutionMode::ASYNC:
            break;

        default:
            return false;
        }

        switch (variant.message_passing())
        {
        case MessagePassing::PUSH:
        case MessagePassing::PULL:
            break;

        default:
            return false;
        }

        switch (variant.scheduling_mode())
        {
        case SchedulingMode::DATA_DRIVEN:
        case SchedulingMode::TOPOLOGY_DRIVEN:
            break;

        default:
            return false;
        }

        return true;
    }

    /**
     * Return whether a frontier is required by the selected variant.
     *
     * Only DATA_DRIVEN execution requires a frontier at this abstraction level.
     */
    constexpr bool requires_frontier(
        const SEPExecutionVariant &variant) noexcept
    {
        return variant.scheduling_mode() ==
               SchedulingMode::DATA_DRIVEN;
    }

    /**
     * Return whether the selected variant is topology-driven.
     */
    constexpr bool is_topology_driven(
        const SEPExecutionVariant &variant) noexcept
    {
        return variant.scheduling_mode() ==
               SchedulingMode::TOPOLOGY_DRIVEN;
    }

} // namespace hytgraph::sep

#endif // HYTGRAPH_SEP_EXECUTION_REQUIREMENTS_HPP