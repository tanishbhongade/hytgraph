#ifndef HYTGRAPH_SEP_EXECUTION_FACTORY_HPP
#define HYTGRAPH_SEP_EXECUTION_FACTORY_HPP

#include <memory>
#include <stdexcept>
#include <string_view>

#include "sep_execution_driver_base.hpp"
#include "sep_execution_requirements.hpp"

namespace hytgraph::sep
{

    /**
     * Phase 12 placeholder driver.
     *
     * This class represents a selected SEP execution variant without performing
     * actual graph execution.
     *
     * It exists so the factory can provide a concrete object for every valid
     * SEP variant while keeping GPU execution deferred to Phase 13.
     *
     * Phase 13 will replace these placeholders with concrete CUDA-backed
     * implementations.
     */
    template <typename TValue,
              typename TBuffer,
              typename TWeight = double>
    class SEPDeferredExecutionDriver final
        : public SEPExecutionDriverBase<TValue, TBuffer, TWeight>
    {
    public:
        using base_type =
            SEPExecutionDriverBase<TValue, TBuffer, TWeight>;

        using step_result_type =
            typename base_type::step_result_type;

        explicit SEPDeferredExecutionDriver(
            SEPExecutionVariant variant)
            : variant_(variant) {}

        void initialize(
            typename base_type::application_type &application,
            const typename base_type::execution_config_type &config) override
        {
            if (config.variant() != variant_)
            {
                throw std::invalid_argument(
                    "SEP execution factory variant does not match configuration");
            }

            base_type::initialize(application, config);
        }

        void set_context(
            SEPExecutionContext &context) noexcept override
        {
            base_type::set_context(context);
        }

        /**
         * Phase 12 deliberately does not execute the graph.
         *
         * Returning NOT_INITIALIZED / INVALID_CONFIGURATION here would make
         * this placeholder misleading, so use INVALID_CONFIGURATION to signal
         * that the selected execution variant has no concrete implementation
         * in Phase 12.
         */
        step_result_type execute_step() override
        {
            return step_result_type::invalid_configuration();
        }

        const typename base_type::execution_variant_type &
        execution_variant() const noexcept override
        {
            return variant_;
        }

    private:
        SEPExecutionVariant variant_;
    };

    /**
     * Factory for SEP execution drivers.
     *
     * Every one of SEP's eight execution combinations is accepted.
     *
     * The factory is deliberately deterministic:
     *
     *   same variant → same driver type/variant selection
     *
     * No heuristic runtime selection is performed here.
     *
     * Automatic runtime switching belongs to a later phase and must not be
     * introduced into the Phase 12 foundation.
     */
    template <typename TValue,
              typename TBuffer,
              typename TWeight = double>
    class SEPExecutionFactory
    {
    public:
        using driver_type =
            SEPExecutionDriver<TValue, TBuffer, TWeight>;

        using driver_ptr =
            std::unique_ptr<driver_type>;

        /**
         * Create a driver for an explicitly selected SEP variant.
         *
         * All eight combinations are structurally valid.
         */
        static driver_ptr create(
            const SEPExecutionVariant &variant)
        {
            if (!is_valid_variant(variant))
            {
                throw std::invalid_argument(
                    "Invalid SEP execution variant");
            }

            return std::make_unique<
                SEPDeferredExecutionDriver<
                    TValue,
                    TBuffer,
                    TWeight>>(variant);
        }

        /**
         * Create a driver directly from execution configuration.
         */
        static driver_ptr create(
            const SEPExecutionConfig &config)
        {
            return create(config.variant());
        }

        /**
         * Create a driver from SEP's canonical variant string.
         *
         * Examples:
         *
         *   SYNC_PUSH_DD
         *   SYNC_PULL_TD
         *   ASYNC_PUSH_DD
         *   ASYNC_PULL_TD
         */
        static driver_ptr create(
            std::string_view variant_name)
        {
            return create(
                SEPExecutionVariant::from_string(variant_name));
        }
    };

} // namespace hytgraph::sep

#endif // HYTGRAPH_SEP_EXECUTION_FACTORY_HPP