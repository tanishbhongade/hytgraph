#ifndef HYTGRAPH_SEP_EXECUTION_SELECTION_HPP
#define HYTGRAPH_SEP_EXECUTION_SELECTION_HPP

#include <string_view>

#include "sep_execution_plan.hpp"
#include "sep_execution_variant_registry.hpp"

namespace hytgraph::sep
{

    /**
     * Explicit SEP execution-variant selection.
     *
     * Phase 12 intentionally uses explicit selection:
     *
     *     configuration -> selected variant -> execution plan
     *
     * There is no runtime autotuning, performance model, workload sampling,
     * variant switching, or adaptive policy selection here.
     *
     * Those mechanisms are execution/runtime concerns and are intentionally
     * deferred.
     */
    class SEPExecutionSelection
    {
    public:
        /**
         * Select a variant directly.
         */
        static constexpr SEPExecutionPlan select(
            const SEPExecutionVariant &variant) noexcept
        {
            return SEPExecutionPlan(
                SEPExecutionConfig(variant));
        }

        /**
         * Select a variant from an execution configuration.
         */
        static constexpr SEPExecutionPlan select(
            const SEPExecutionConfig &config) noexcept
        {
            return SEPExecutionPlan(config);
        }

        /**
         * Select a variant using SEP's canonical string representation.
         *
         * Unlike registry lookup, invalid names are rejected by
         * SEPExecutionVariant::from_string().
         */
        static SEPExecutionPlan select(
            std::string_view variant_name)
        {
            return SEPExecutionPlan(
                SEPExecutionConfig::from_string(variant_name));
        }

        /**
         * Select the canonical default variant.
         *
         * The default is SYNC_PUSH_DD, matching the default policy established
         * by SEPExecutionConfig.
         */
        static constexpr SEPExecutionPlan select_default() noexcept
        {
            return SEPExecutionPlan(
                SEPExecutionConfig{});
        }

        /**
         * Check whether a variant name can be selected.
         *
         * This is non-throwing and is useful for configuration validation.
         */
        static bool can_select(
            std::string_view variant_name) noexcept
        {
            return SEPExecutionVariantRegistry::contains(
                variant_name);
        }
    };

} // namespace hytgraph::sep

#endif // HYTGRAPH_SEP_EXECUTION_SELECTION_HPP