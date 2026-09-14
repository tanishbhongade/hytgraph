#include <cassert>

#include "sep/sep_application.hpp"
#include "sep/sep_execution_variant.hpp"

namespace
{

    using namespace hytgraph::sep;

    /*
     * Minimal application implementation used only to validate the Phase 12
     * application/execution boundary.
     *
     * This is deliberately algorithm-neutral. It does not depend on the project's
     * CSR graph, CUDA implementation, partitions, or transfer layer.
     */
    class TestApplication final
        : public SEPApplication<double, double, double>
    {
    public:
        TestApplication()
            : variant_{
                  ExecutionMode::SYNC,
                  MessagePassing::PUSH,
                  SchedulingMode::DATA_DRIVEN}
        {
        }

        void set_execution_variant(
            const SEPExecutionVariant &variant) override
        {
            variant_ = variant;
        }

        const SEPExecutionVariant &
        execution_variant() const noexcept override
        {
            return variant_;
        }

        double init_value(
            node_id_type node) const override
        {
            return static_cast<double>(node);
        }

        double init_buffer(
            node_id_type) const override
        {
            return identity_element();
        }

        double identity_element() const override
        {
            return 0.0;
        }

        CombineResult combine_value_buffer(
            node_id_type,
            const double &value,
            const double &buffer) const override
        {
            return CombineResult{
                value + buffer,
                buffer != 0.0};
        }

        AccumulateResult accumulate_buffer(
            node_id_type,
            node_id_type,
            double &destination,
            const double &message) const override
        {
            const double old_value = destination;

            destination += message;

            return AccumulateResult{
                destination != old_value,
                message != 0.0};
        }

        AccumulateResult accumulate_buffer(
            node_id_type,
            node_id_type,
            double weight,
            double &destination,
            const double &message) const override
        {
            const double weighted_message =
                weight * message;

            const double old_value = destination;

            destination += weighted_message;

            return AccumulateResult{
                destination != old_value,
                weighted_message != 0.0};
        }

        bool is_active(
            node_id_type,
            const double &buffer) const override
        {
            return buffer != 0.0;
        }

    private:
        SEPExecutionVariant variant_;
    };

    void test_application_can_be_constructed()
    {
        TestApplication application;

        assert(
            application.execution_variant().execution_mode() ==
            ExecutionMode::SYNC);

        assert(
            application.execution_variant().message_passing() ==
            MessagePassing::PUSH);

        assert(
            application.execution_variant().scheduling_mode() ==
            SchedulingMode::DATA_DRIVEN);
    }

    void test_variant_can_be_changed()
    {
        TestApplication application;

        const SEPExecutionVariant variant{
            ExecutionMode::ASYNC,
            MessagePassing::PULL,
            SchedulingMode::TOPOLOGY_DRIVEN};

        application.set_execution_variant(variant);

        assert(
            application.execution_variant() == variant);
    }

    void test_initialization_contract()
    {
        TestApplication application;

        assert(application.init_value(7) == 7.0);
        assert(application.init_buffer(7) == 0.0);
        assert(application.identity_element() == 0.0);
    }

    void test_combine_contract()
    {
        TestApplication application;

        const auto result =
            application.combine_value_buffer(
                0,
                2.0,
                3.0);

        assert(result.new_buffer == 5.0);
        assert(result.activate);
    }

    void test_combine_zero_buffer()
    {
        TestApplication application;

        const auto result =
            application.combine_value_buffer(
                0,
                2.0,
                0.0);

        assert(result.new_buffer == 2.0);
        assert(!result.activate);
    }

    void test_accumulate_contract()
    {
        TestApplication application;

        double destination = 1.0;

        const auto result =
            application.accumulate_buffer(
                0,
                1,
                destination,
                2.5);

        assert(destination == 3.5);
        assert(result.changed);
        assert(result.activate);
    }

    void test_weighted_accumulate_contract()
    {
        TestApplication application;

        double destination = 1.0;

        const auto result =
            application.accumulate_buffer(
                0,
                1,
                4.0,
                destination,
                2.0);

        assert(destination == 9.0);
        assert(result.changed);
        assert(result.activate);
    }

    void test_activity_contract()
    {
        TestApplication application;

        assert(application.is_active(0, 1.0));
        assert(!application.is_active(0, 0.0));
    }

    void test_default_application_hooks()
    {
        TestApplication application;

        application.post_computation();

        assert(
            application.is_high_priority(
                0.0,
                1.0));

        assert(!application.requires_edge_weights());
        assert(application.supports_activity_predicate());
    }

    void test_polymorphic_use()
    {
        TestApplication concrete;

        SEPApplication<double, double, double> &application =
            concrete;

        const SEPExecutionVariant variant{
            ExecutionMode::ASYNC,
            MessagePassing::PULL,
            SchedulingMode::DATA_DRIVEN};

        application.set_execution_variant(variant);

        assert(
            application.execution_variant() == variant);

        assert(
            application.identity_element() == 0.0);
    }

} // namespace

int main()
{
    test_application_can_be_constructed();
    test_variant_can_be_changed();
    test_initialization_contract();
    test_combine_contract();
    test_combine_zero_buffer();
    test_accumulate_contract();
    test_weighted_accumulate_contract();
    test_activity_contract();
    test_default_application_hooks();
    test_polymorphic_use();

    return 0;
}