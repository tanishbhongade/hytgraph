#ifndef HYTGRAPH_SEP_EXECUTION_DRIVER_BASE_HPP
#define HYTGRAPH_SEP_EXECUTION_DRIVER_BASE_HPP

#include <stdexcept>

#include "sep_execution_driver.hpp"
#include "sep_execution_requirements.hpp"

namespace hytgraph::sep
{

    /**
     * Shared non-GPU implementation support for SEP execution drivers.
     *
     * This class centralizes the state common to future concrete execution
     * variants:
     *
     *   - application binding;
     *   - execution configuration;
     *   - execution context;
     *   - initialization state;
     *   - convergence state.
     *
     * It does NOT perform graph traversal or message passing.
     *
     * Concrete Phase 13 drivers will derive from this class and implement the
     * actual execution step.
     */
    template <typename TValue,
              typename TBuffer,
              typename TWeight = double>
    class SEPExecutionDriverBase
        : public SEPExecutionDriver<TValue, TBuffer, TWeight>
    {
    public:
        using base_type =
            SEPExecutionDriver<TValue, TBuffer, TWeight>;

        using application_type =
            typename base_type::application_type;

        using execution_config_type =
            typename base_type::execution_config_type;

        using execution_variant_type =
            typename base_type::execution_variant_type;

        using step_result_type =
            typename base_type::step_result_type;

        SEPExecutionDriverBase() = default;

        ~SEPExecutionDriverBase() override = default;

        /**
         * Bind application and execution state.
         *
         * This performs only Phase 12 structural validation.
         *
         * No CUDA state is created and no graph execution occurs.
         */
        void initialize(
            application_type &application,
            const execution_config_type &config) override
        {
            if (!is_valid_variant(config.variant()))
            {
                throw std::invalid_argument(
                    "Invalid SEP execution variant");
            }

            application_ = &application;
            config_ = config;
            initialized_ = true;
            converged_ = false;

            application_->set_execution_variant(config.variant());
        }

        /**
         * Concrete execution variants implement this method in later phases.
         *
         * Keeping the method abstract prevents Phase 12 from accidentally
         * becoming a CPU/GPU implementation of SEP.
         */
        step_result_type execute_step() override = 0;

        /**
         * Return the selected SEP execution variant.
         */
        const execution_variant_type &
        execution_variant() const noexcept override
        {
            return config_.variant();
        }

        /**
         * Return whether initialization has occurred.
         */
        bool initialized() const noexcept override
        {
            return initialized_;
        }

        /**
         * Return whether the application has reached convergence.
         */
        bool converged() const noexcept override
        {
            return converged_;
        }

        /**
         * Release non-owning execution state.
         *
         * No graph/activity/task object is destroyed because this class does not
         * own project state.
         */
        void finalize() noexcept override
        {
            application_ = nullptr;
            initialized_ = false;
            converged_ = false;
            context_ = nullptr;
        }

    protected:
        /**
         * Attach the existing project's execution context.
         *
         * This is deliberately protected so concrete drivers can bind graph,
         * activity, partition, task, and frontier state without changing the
         * public SEP driver interface.
         */
        void set_context(
            SEPExecutionContext &context) noexcept override
        {
            context_ = &context;
        }

        /**
         * Access the currently bound application.
         *
         * The caller must ensure initialized() is true.
         */
        application_type &application()
        {
            if (application_ == nullptr)
            {
                throw std::logic_error(
                    "SEP execution application is not initialized");
            }

            return *application_;
        }

        const application_type &application() const
        {
            if (application_ == nullptr)
            {
                throw std::logic_error(
                    "SEP execution application is not initialized");
            }

            return *application_;
        }

        /**
         * Access the execution configuration.
         */
        const execution_config_type &config() const noexcept
        {
            return config_;
        }

        /**
         * Access the currently bound execution context.
         */
        SEPExecutionContext &context()
        {
            if (context_ == nullptr)
            {
                throw std::logic_error(
                    "SEP execution context is not initialized");
            }

            return *context_;
        }

        const SEPExecutionContext &context() const
        {
            if (context_ == nullptr)
            {
                throw std::logic_error(
                    "SEP execution context is not initialized");
            }

            return *context_;
        }

        /**
         * Validate the current application/context binding.
         *
         * The graph handle is opaque at this phase, but a concrete context must
         * still explicitly indicate that graph state exists.
         */
        bool context_is_valid() const noexcept
        {
            if (!initialized_ ||
                application_ == nullptr ||
                context_ == nullptr)
            {
                return false;
            }

            return is_valid_execution_configuration(
                config_,
                *context_);
        }

        /**
         * Mark the current execution as converged.
         *
         * Concrete Phase 13 implementations can invoke this when their
         * application-level termination criterion is satisfied.
         */
        void mark_converged() noexcept
        {
            converged_ = true;
        }

        /**
         * Mark execution as active again.
         *
         * Useful if a future execution step resumes after previously reaching a
         * provisional convergence state.
         */
        void clear_converged() noexcept
        {
            converged_ = false;
        }

    private:
        application_type *application_{nullptr};

        execution_config_type config_{};

        SEPExecutionContext *context_{nullptr};

        bool initialized_{false};
        bool converged_{false};
    };

} // namespace hytgraph::sep

#endif // HYTGRAPH_SEP_EXECUTION_DRIVER_BASE_HPP