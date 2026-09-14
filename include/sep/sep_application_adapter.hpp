#ifndef HYTGRAPH_SEP_APPLICATION_ADAPTER_HPP
#define HYTGRAPH_SEP_APPLICATION_ADAPTER_HPP

#include <cstddef>
#include <stdexcept>

#include "sep_application.hpp"

namespace hytgraph::sep
{

    /**
     * Non-owning adapter from an existing application implementation to the
     * SEPApplication contract.
     *
     * Phase 12:
     *   - defines the boundary between an existing PageRank/SSSP implementation
     *     and the SEP execution layer;
     *   - does not copy or own the wrapped application;
     *   - does not launch CPU/GPU execution.
     *
     * The wrapped application must already provide the semantic operations
     * required by SEPApplication.
     *
     * This adapter intentionally does not impose a new inheritance hierarchy
     * on the project's existing PageRank/SSSP classes.
     */
    template <typename Application>
    class SEPApplicationAdapter final
        : public SEPApplication<
              typename Application::value_type,
              typename Application::buffer_type,
              typename Application::weight_type>
    {
    public:
        using application_type = Application;

        using value_type =
            typename Application::value_type;

        using buffer_type =
            typename Application::buffer_type;

        using weight_type =
            typename Application::weight_type;

        using base_type =
            SEPApplication<
                value_type,
                buffer_type,
                weight_type>;

        using node_id_type =
            typename base_type::node_id_type;

        using CombineResult =
            typename base_type::CombineResult;

        using AccumulateResult =
            typename base_type::AccumulateResult;

        explicit SEPApplicationAdapter(
            Application &application) noexcept
            : application_(&application)
        {
        }

        ~SEPApplicationAdapter() override = default;

        SEPApplicationAdapter(
            const SEPApplicationAdapter &) = delete;

        SEPApplicationAdapter &operator=(
            const SEPApplicationAdapter &) = delete;

        /**
         * Bind the selected SEP execution variant.
         *
         * The wrapped application remains responsible for interpreting the
         * variant. This adapter only forwards the configuration.
         */
        void set_execution_variant(
            const SEPExecutionVariant &variant) override
        {
            application_->set_execution_variant(variant);
        }

        const SEPExecutionVariant &execution_variant()
            const noexcept override
        {
            return application_->execution_variant();
        }

        value_type init_value(
            node_id_type node) const override
        {
            return application_->init_value(node);
        }

        buffer_type init_buffer(
            node_id_type node) const override
        {
            return application_->init_buffer(node);
        }

        buffer_type identity_element() const override
        {
            return application_->identity_element();
        }

        CombineResult combine_value_buffer(
            node_id_type node,
            const value_type &value,
            const buffer_type &buffer) const override
        {
            return application_->combine_value_buffer(
                node,
                value,
                buffer);
        }

        AccumulateResult accumulate_buffer(
            node_id_type src,
            node_id_type dst,
            buffer_type &destination,
            const buffer_type &message) const override
        {
            return application_->accumulate_buffer(
                src,
                dst,
                destination,
                message);
        }

        AccumulateResult accumulate_buffer(
            node_id_type src,
            node_id_type dst,
            weight_type weight,
            buffer_type &destination,
            const buffer_type &message) const override
        {
            return application_->accumulate_buffer(
                src,
                dst,
                weight,
                destination,
                message);
        }

        bool is_active(
            node_id_type node,
            const buffer_type &buffer) const override
        {
            return application_->is_active(
                node,
                buffer);
        }

        void post_computation() override
        {
            application_->post_computation();
        }

        bool is_high_priority(
            const buffer_type &current_priority,
            const buffer_type &buffer) const override
        {
            return application_->is_high_priority(
                current_priority,
                buffer);
        }

        bool requires_edge_weights()
            const noexcept override
        {
            return application_->requires_edge_weights();
        }

        bool supports_activity_predicate()
            const noexcept override
        {
            return application_->supports_activity_predicate();
        }

        /**
         * Access the existing application.
         *
         * Ownership remains with the caller.
         */
        Application &underlying() noexcept
        {
            return *application_;
        }

        const Application &underlying() const noexcept
        {
            return *application_;
        }

    private:
        Application *application_;
    };

    /**
     * Convenience factory for creating a non-owning SEP application adapter.
     */
    template <typename Application>
    SEPApplicationAdapter<Application>
    make_sep_application_adapter(
        Application &application) noexcept
    {
        return SEPApplicationAdapter<Application>(
            application);
    }

} // namespace hytgraph::sep

#endif // HYTGRAPH_SEP_APPLICATION_ADAPTER_HPP