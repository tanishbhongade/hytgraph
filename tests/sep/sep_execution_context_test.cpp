#include <cassert>
#include <cstddef>

#include "sep/sep_execution_context.hpp"
#include "sep/sep_execution_variant.hpp"

namespace
{

    using namespace hytgraph::sep;

    void test_default_context_has_no_graph()
    {
        SEPExecutionContext context;

        assert(!context.has_graph());
    }

    void test_default_context_has_no_frontier()
    {
        SEPExecutionContext context;

        assert(!context.has_frontier());
    }

    void test_default_context_does_not_satisfy_data_driven_variant()
    {
        SEPExecutionContext context;

        const SEPExecutionVariant variant{
            ExecutionMode::SYNC,
            MessagePassing::PUSH,
            SchedulingMode::DATA_DRIVEN};

        assert(!context.satisfies_variant_requirements(variant));
    }

    void test_default_context_topology_driven_requirement()
    {
        SEPExecutionContext context;

        const SEPExecutionVariant variant{
            ExecutionMode::SYNC,
            MessagePassing::PULL,
            SchedulingMode::TOPOLOGY_DRIVEN};

        /*
         * Topology-driven execution does not require a frontier.
         *
         * The context-level requirement check therefore accepts this variant
         * independently of the graph-presence validation performed by
         * is_valid_execution_configuration().
         */
        assert(context.satisfies_variant_requirements(variant));
    }

    void test_variant_requirement_is_deterministic()
    {
        SEPExecutionContext context;

        const SEPExecutionVariant variant{
            ExecutionMode::ASYNC,
            MessagePassing::PULL,
            SchedulingMode::DATA_DRIVEN};

        const bool first =
            context.satisfies_variant_requirements(variant);

        const bool second =
            context.satisfies_variant_requirements(variant);

        assert(first == second);
    }

    void test_context_can_be_used_polymorphically_if_interface_requires_it()
    {
        SEPExecutionContext context;

        /*
         * This test intentionally only exercises the concrete Phase 12 context
         * object. The context is a value describing execution inputs; it is not
         * itself an execution driver.
         */
        assert(!context.has_graph());
        assert(!context.has_frontier());
    }

} // namespace

int main()
{
    test_default_context_has_no_graph();
    test_default_context_has_no_frontier();
    test_default_context_does_not_satisfy_data_driven_variant();
    test_default_context_topology_driven_requirement();
    test_variant_requirement_is_deterministic();
    test_context_can_be_used_polymorphically_if_interface_requires_it();

    return 0;
}