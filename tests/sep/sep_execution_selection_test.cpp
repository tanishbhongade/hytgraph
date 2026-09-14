#include <cassert>
#include <string>

#include "sep/sep_execution_selection.hpp"

namespace
{

    using namespace hytgraph::sep;

    void test_select_by_variant()
    {
        const SEPExecutionVariant variant(
            ExecutionMode::ASYNC,
            MessagePassing::PULL,
            SchedulingMode::DATA_DRIVEN);

        const auto plan =
            SEPExecutionSelection::select(variant);

        assert(plan.variant() == variant);
        assert(plan.to_string() == "ASYNC_PULL_DD");

        assert(plan.is_async());
        assert(plan.is_pull());
        assert(plan.is_data_driven());
    }

    void test_select_by_configuration()
    {
        const SEPExecutionConfig config(
            SEPExecutionVariant(
                ExecutionMode::SYNC,
                MessagePassing::PUSH,
                SchedulingMode::TOPOLOGY_DRIVEN));

        const auto plan =
            SEPExecutionSelection::select(config);

        assert(plan.variant() == config.variant());
        assert(plan.to_string() == "SYNC_PUSH_TD");

        assert(plan.is_sync());
        assert(plan.is_push());
        assert(plan.is_topology_driven());
    }

    void test_select_by_name()
    {
        const auto plan =
            SEPExecutionSelection::select(
                "ASYNC_PULL_TD");

        assert(plan.to_string() ==
               "ASYNC_PULL_TD");

        assert(plan.is_async());
        assert(plan.is_pull());
        assert(plan.is_topology_driven());
    }

    void test_default_selection()
    {
        const auto plan =
            SEPExecutionSelection::select_default();

        assert(plan.to_string() ==
               "SYNC_PUSH_DD");

        assert(plan.is_sync());
        assert(plan.is_push());
        assert(plan.is_data_driven());
    }

    void test_can_select()
    {
        assert(
            SEPExecutionSelection::can_select(
                "SYNC_PUSH_DD"));

        assert(
            SEPExecutionSelection::can_select(
                "SYNC_PUSH_TD"));

        assert(
            SEPExecutionSelection::can_select(
                "SYNC_PULL_DD"));

        assert(
            SEPExecutionSelection::can_select(
                "SYNC_PULL_TD"));

        assert(
            SEPExecutionSelection::can_select(
                "ASYNC_PUSH_DD"));

        assert(
            SEPExecutionSelection::can_select(
                "ASYNC_PUSH_TD"));

        assert(
            SEPExecutionSelection::can_select(
                "ASYNC_PULL_DD"));

        assert(
            SEPExecutionSelection::can_select(
                "ASYNC_PULL_TD"));

        assert(
            !SEPExecutionSelection::can_select(
                "INVALID_VARIANT"));

        assert(
            !SEPExecutionSelection::can_select(
                ""));

        assert(
            !SEPExecutionSelection::can_select(
                "SYNC_PUSH"));
    }

    void test_selection_is_deterministic()
    {
        const auto first =
            SEPExecutionSelection::select(
                "ASYNC_PUSH_TD");

        const auto second =
            SEPExecutionSelection::select(
                "ASYNC_PUSH_TD");

        assert(first.variant() == second.variant());
        assert(first.to_string() == second.to_string());

        assert(first.is_async() == second.is_async());
        assert(first.is_push() == second.is_push());
        assert(first.is_topology_driven() ==
               second.is_topology_driven());
    }

    void test_invalid_name_is_rejected()
    {
        bool threw = false;

        try
        {
            (void)SEPExecutionSelection::select(
                "NOT_A_SEP_VARIANT");
        }
        catch (...)
        {
            threw = true;
        }

        assert(threw);
    }

} // namespace

int main()
{
    test_select_by_variant();
    test_select_by_configuration();
    test_select_by_name();
    test_default_selection();
    test_can_select();
    test_selection_is_deterministic();
    test_invalid_name_is_rejected();

    return 0;
}