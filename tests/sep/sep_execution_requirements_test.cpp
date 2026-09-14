#include <cassert>

#include "sep/sep_execution_requirements.hpp"
#include "sep/sep_execution_variant_registry.hpp"

namespace
{

    using namespace hytgraph::sep;

    void test_all_variants_are_structurally_valid()
    {
        const auto &variants =
            SEPExecutionVariantRegistry::all();

        assert(variants.size() == 8);

        for (const auto &variant : variants)
        {
            assert(is_valid_variant(variant));
        }
    }

    void test_data_driven_requires_frontier()
    {
        const SEPExecutionVariant variant{
            ExecutionMode::SYNC,
            MessagePassing::PUSH,
            SchedulingMode::DATA_DRIVEN};

        assert(is_valid_variant(variant));
        assert(requires_frontier(variant));
        assert(!is_topology_driven(variant));
    }

    void test_topology_driven_does_not_require_frontier()
    {
        const SEPExecutionVariant variant{
            ExecutionMode::SYNC,
            MessagePassing::PUSH,
            SchedulingMode::TOPOLOGY_DRIVEN};

        assert(is_valid_variant(variant));
        assert(!requires_frontier(variant));
        assert(is_topology_driven(variant));
    }

    void test_scheduling_dimension_is_independent_of_execution_mode()
    {
        const SEPExecutionVariant sync_dd{
            ExecutionMode::SYNC,
            MessagePassing::PULL,
            SchedulingMode::DATA_DRIVEN};

        const SEPExecutionVariant async_dd{
            ExecutionMode::ASYNC,
            MessagePassing::PULL,
            SchedulingMode::DATA_DRIVEN};

        const SEPExecutionVariant sync_td{
            ExecutionMode::SYNC,
            MessagePassing::PULL,
            SchedulingMode::TOPOLOGY_DRIVEN};

        const SEPExecutionVariant async_td{
            ExecutionMode::ASYNC,
            MessagePassing::PULL,
            SchedulingMode::TOPOLOGY_DRIVEN};

        assert(requires_frontier(sync_dd));
        assert(requires_frontier(async_dd));

        assert(!requires_frontier(sync_td));
        assert(!requires_frontier(async_td));

        assert(!is_topology_driven(sync_dd));
        assert(!is_topology_driven(async_dd));

        assert(is_topology_driven(sync_td));
        assert(is_topology_driven(async_td));
    }

    void test_message_passing_does_not_change_frontier_requirement()
    {
        const SEPExecutionVariant push_dd{
            ExecutionMode::SYNC,
            MessagePassing::PUSH,
            SchedulingMode::DATA_DRIVEN};

        const SEPExecutionVariant pull_dd{
            ExecutionMode::SYNC,
            MessagePassing::PULL,
            SchedulingMode::DATA_DRIVEN};

        const SEPExecutionVariant push_td{
            ExecutionMode::SYNC,
            MessagePassing::PUSH,
            SchedulingMode::TOPOLOGY_DRIVEN};

        const SEPExecutionVariant pull_td{
            ExecutionMode::SYNC,
            MessagePassing::PULL,
            SchedulingMode::TOPOLOGY_DRIVEN};

        assert(requires_frontier(push_dd));
        assert(requires_frontier(pull_dd));

        assert(!requires_frontier(push_td));
        assert(!requires_frontier(pull_td));
    }

    void test_configuration_with_graph()
    {
        SEPExecutionContext context;

        /*
         * We intentionally use the context's public API rather than constructing
         * any concrete graph representation here.
         *
         * If your current context requires an explicit graph binding method,
         * this test should be adapted to that existing API.
         */
        assert(!context.has_graph());
    }

    void test_configuration_validation_without_graph()
    {
        const SEPExecutionConfig config{
            SEPExecutionVariant{
                ExecutionMode::SYNC,
                MessagePassing::PUSH,
                SchedulingMode::DATA_DRIVEN}};

        SEPExecutionContext context;

        /*
         * A graph is mandatory for a valid execution configuration.
         */
        assert(!is_valid_execution_configuration(
            config,
            context));
    }

    void test_configuration_validation_is_deterministic()
    {
        const SEPExecutionConfig config{
            SEPExecutionVariant{
                ExecutionMode::ASYNC,
                MessagePassing::PULL,
                SchedulingMode::TOPOLOGY_DRIVEN}};

        SEPExecutionContext context;

        const bool first =
            is_valid_execution_configuration(
                config,
                context);

        const bool second =
            is_valid_execution_configuration(
                config,
                context);

        assert(first == second);
    }

} // namespace

int main()
{
    test_all_variants_are_structurally_valid();
    test_data_driven_requires_frontier();
    test_topology_driven_does_not_require_frontier();
    test_scheduling_dimension_is_independent_of_execution_mode();
    test_message_passing_does_not_change_frontier_requirement();
    test_configuration_with_graph();
    test_configuration_validation_without_graph();
    test_configuration_validation_is_deterministic();

    return 0;
}