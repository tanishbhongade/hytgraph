#include <cassert>

#include "sep/sep_execution_plan.hpp"

namespace
{

    using namespace hytgraph::sep;

    void test_sync_push_data_driven()
    {
        const SEPExecutionVariant variant{
            ExecutionMode::SYNC,
            MessagePassing::PUSH,
            SchedulingMode::DATA_DRIVEN};

        const SEPExecutionPlan plan{
            SEPExecutionConfig{variant}};

        assert(plan.variant() == variant);
        assert(plan.to_string() == "SYNC_PUSH_DD");

        assert(plan.is_sync());
        assert(!plan.is_async());

        assert(plan.is_push());
        assert(!plan.is_pull());

        assert(plan.is_data_driven());
        assert(!plan.is_topology_driven());
    }

    void test_sync_pull_topology_driven()
    {
        const SEPExecutionVariant variant{
            ExecutionMode::SYNC,
            MessagePassing::PULL,
            SchedulingMode::TOPOLOGY_DRIVEN};

        const SEPExecutionPlan plan{
            SEPExecutionConfig{variant}};

        assert(plan.variant() == variant);
        assert(plan.to_string() == "SYNC_PULL_TD");

        assert(plan.is_sync());
        assert(plan.is_pull());
        assert(plan.is_topology_driven());
    }

    void test_async_push_topology_driven()
    {
        const SEPExecutionVariant variant{
            ExecutionMode::ASYNC,
            MessagePassing::PUSH,
            SchedulingMode::TOPOLOGY_DRIVEN};

        const SEPExecutionPlan plan{
            SEPExecutionConfig{variant}};

        assert(plan.variant() == variant);
        assert(plan.to_string() == "ASYNC_PUSH_TD");

        assert(plan.is_async());
        assert(plan.is_push());
        assert(plan.is_topology_driven());
    }

    void test_async_pull_data_driven()
    {
        const SEPExecutionVariant variant{
            ExecutionMode::ASYNC,
            MessagePassing::PULL,
            SchedulingMode::DATA_DRIVEN};

        const SEPExecutionPlan plan{
            SEPExecutionConfig{variant}};

        assert(plan.variant() == variant);
        assert(plan.to_string() == "ASYNC_PULL_DD");

        assert(plan.is_async());
        assert(plan.is_pull());
        assert(plan.is_data_driven());
    }

    void test_default_plan()
    {
        const SEPExecutionPlan plan{
            SEPExecutionConfig{}};

        assert(plan.to_string() == "SYNC_PUSH_DD");

        assert(plan.is_sync());
        assert(plan.is_push());
        assert(plan.is_data_driven());
    }

    void test_plan_preserves_configuration()
    {
        const SEPExecutionConfig config{
            SEPExecutionVariant{
                ExecutionMode::ASYNC,
                MessagePassing::PULL,
                SchedulingMode::TOPOLOGY_DRIVEN}};

        const SEPExecutionPlan plan{config};

        assert(plan.config().variant() == config.variant());
        assert(plan.variant() == config.variant());

        assert(plan.to_string() ==
               config.variant().to_string());
    }

    void test_frontier_requirement()
    {
        const SEPExecutionPlan data_driven{
            SEPExecutionConfig{
                SEPExecutionVariant{
                    ExecutionMode::SYNC,
                    MessagePassing::PUSH,
                    SchedulingMode::DATA_DRIVEN}}};

        const SEPExecutionPlan topology_driven{
            SEPExecutionConfig{
                SEPExecutionVariant{
                    ExecutionMode::SYNC,
                    MessagePassing::PUSH,
                    SchedulingMode::TOPOLOGY_DRIVEN}}};

        assert(data_driven.requires_frontier());
        assert(!topology_driven.requires_frontier());
    }

    void test_plan_copy()
    {
        const SEPExecutionPlan original{
            SEPExecutionConfig{
                SEPExecutionVariant{
                    ExecutionMode::ASYNC,
                    MessagePassing::PULL,
                    SchedulingMode::DATA_DRIVEN}}};

        const SEPExecutionPlan copy{original};

        assert(copy.variant() == original.variant());
        assert(copy.to_string() == original.to_string());

        assert(copy.is_async() == original.is_async());
        assert(copy.is_pull() == original.is_pull());
        assert(copy.is_data_driven() ==
               original.is_data_driven());
    }

} // namespace

int main()
{
    test_sync_push_data_driven();
    test_sync_pull_topology_driven();
    test_async_push_topology_driven();
    test_async_pull_data_driven();
    test_default_plan();
    test_plan_preserves_configuration();
    test_frontier_requirement();
    test_plan_copy();

    return 0;
}