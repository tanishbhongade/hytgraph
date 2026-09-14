#ifndef HYTGRAPH_SEP_EXECUTION_PLAN_HPP
#define HYTGRAPH_SEP_EXECUTION_PLAN_HPP

#include "sep_execution_config.hpp"
#include "sep_execution_context.hpp"
#include "sep_execution_requirements.hpp"

namespace hytgraph::sep
{

    /**
     * Immutable execution plan for one selected SEP variant.
     *
     * The plan is the boundary between configuration and execution.
     *
     * It does not execute anything and does not own any graph/runtime state.
     *
     * Conceptually:
     *
     *     graph + application + configuration
     *                    |
     *                    v
     *            SEPExecutionPlan
     *                    |
     *                    v
     *            SEPExecutionDriver
     *
     * The plan records only the execution decisions that the future driver needs:
     *
     *     SYNC / ASYNC
     *     PUSH / PULL
     *     DATA-DRIVEN / TOPOLOGY-DRIVEN
     *
     * This mirrors SEP's separation of execution dimensions while avoiding the
     * larger SEP GraphDatum/runtime structure.
     */
    class SEPExecutionPlan
    {
    public:
        /**
         * Construct an execution plan from an explicit configuration.
         */
        explicit constexpr SEPExecutionPlan(
            SEPExecutionConfig config) noexcept
            : config_(config) {}

        /**
         * Return the selected execution configuration.
         */
        constexpr const SEPExecutionConfig &config() const noexcept
        {
            return config_;
        }

        /**
         * Return the selected execution variant.
         */
        constexpr const SEPExecutionVariant &variant() const noexcept
        {
            return config_.variant();
        }

        constexpr bool is_sync() const noexcept
        {
            return variant().is_synchronous();
        }

        constexpr bool is_async() const noexcept
        {
            return variant().is_asynchronous();
        }

        constexpr bool is_push() const noexcept
        {
            return variant().is_push();
        }

        constexpr bool is_pull() const noexcept
        {
            return variant().is_pull();
        }

        constexpr bool is_data_driven() const noexcept
        {
            return variant().is_data_driven();
        }

        constexpr bool is_topology_driven() const noexcept
        {
            return variant().is_topology_driven();
        }

        /**
         * Whether this execution plan structurally requires a frontier.
         */
        constexpr bool requires_frontier() const noexcept
        {
            return sep::requires_frontier(variant());
        }

        /**
         * Validate this plan against the supplied execution context.
         *
         * This performs structural validation only.
         */
        bool is_compatible_with(
            const SEPExecutionContext &context) const noexcept
        {
            return is_valid_execution_configuration(
                config_,
                context);
        }

        /**
         * Stable representation used by diagnostics and tests.
         */
        std::string to_string() const
        {
            return variant().to_string();
        }

    private:
        SEPExecutionConfig config_;
    };

} // namespace hytgraph::sep

#endif // HYTGRAPH_SEP_EXECUTION_PLAN_HPP