#include <array>
#include <cassert>
#include <string>

#include "sep/sep_execution_variant.hpp"
#include "sep/sep_execution_variant_registry.hpp"

namespace
{

    using namespace hytgraph::sep;

    void test_all_variants_are_registered()
    {
        const auto &variants =
            SEPExecutionVariantRegistry::all();

        assert(variants.size() == 8);
        assert(SEPExecutionVariantRegistry::size() == 8);
    }

    void test_expected_variant_names()
    {
        const std::array<const char *, 8> expected = {
            "SYNC_PUSH_DD",
            "SYNC_PUSH_TD",
            "SYNC_PULL_DD",
            "SYNC_PULL_TD",
            "ASYNC_PUSH_DD",
            "ASYNC_PUSH_TD",
            "ASYNC_PULL_DD",
            "ASYNC_PULL_TD"};

        const auto &variants =
            SEPExecutionVariantRegistry::all();

        for (std::size_t i = 0; i < expected.size(); ++i)
        {
            assert(variants[i].to_string() == expected[i]);
        }
    }

    void test_all_combinations_are_unique()
    {
        const auto &variants =
            SEPExecutionVariantRegistry::all();

        for (std::size_t i = 0; i < variants.size(); ++i)
        {
            for (std::size_t j = i + 1; j < variants.size(); ++j)
            {
                assert(variants[i] != variants[j]);
                assert(
                    variants[i].to_string() !=
                    variants[j].to_string());
            }
        }
    }

    void test_variant_round_trip()
    {
        const auto &variants =
            SEPExecutionVariantRegistry::all();

        for (const auto &variant : variants)
        {
            const auto name = variant.to_string();

            const auto reconstructed =
                SEPExecutionVariant::from_string(name);

            assert(reconstructed == variant);
            assert(reconstructed.to_string() == name);
        }
    }

    void test_variant_dimensions()
    {
        const auto &variants =
            SEPExecutionVariantRegistry::all();

        for (const auto &variant : variants)
        {
            const bool valid_execution_mode =
                variant.execution_mode() == ExecutionMode::SYNC ||
                variant.execution_mode() == ExecutionMode::ASYNC;

            const bool valid_message_mode =
                variant.message_passing() == MessagePassing::PUSH ||
                variant.message_passing() == MessagePassing::PULL;

            const bool valid_schedule_mode =
                variant.scheduling_mode() ==
                    SchedulingMode::DATA_DRIVEN ||
                variant.scheduling_mode() ==
                    SchedulingMode::TOPOLOGY_DRIVEN;

            assert(valid_execution_mode);
            assert(valid_message_mode);
            assert(valid_schedule_mode);
        }
    }

    void test_specific_variants()
    {
        const SEPExecutionVariant sync_push_dd(
            ExecutionMode::SYNC,
            MessagePassing::PUSH,
            SchedulingMode::DATA_DRIVEN);

        assert(sync_push_dd.to_string() ==
               "SYNC_PUSH_DD");

        assert(sync_push_dd.is_synchronous());
        assert(!sync_push_dd.is_asynchronous());

        assert(sync_push_dd.is_push());
        assert(!sync_push_dd.is_pull());

        assert(sync_push_dd.is_data_driven());
        assert(!sync_push_dd.is_topology_driven());

        const SEPExecutionVariant async_pull_td(
            ExecutionMode::ASYNC,
            MessagePassing::PULL,
            SchedulingMode::TOPOLOGY_DRIVEN);

        assert(async_pull_td.to_string() ==
               "ASYNC_PULL_TD");

        assert(async_pull_td.is_asynchronous());
        assert(!async_pull_td.is_synchronous());

        assert(async_pull_td.is_pull());
        assert(!async_pull_td.is_push());

        assert(async_pull_td.is_topology_driven());
        assert(!async_pull_td.is_data_driven());
    }

    void test_invalid_variant_names()
    {
        const std::array<const char *, 5> invalid_names = {
            "",
            "SYNC",
            "PUSH_DD",
            "ASYNC_PUSH",
            "NOT_A_VARIANT"};

        for (const auto *name : invalid_names)
        {
            assert(
                !SEPExecutionVariantRegistry::contains(name));
        }
    }

    void test_registry_lookup_is_deterministic()
    {
        constexpr const char *name =
            "ASYNC_PULL_DD";

        const auto *first =
            SEPExecutionVariantRegistry::find(name);

        const auto *second =
            SEPExecutionVariantRegistry::find(name);

        assert(first != nullptr);
        assert(second != nullptr);

        assert(*first == *second);
        assert(first->to_string() == second->to_string());
    }

} // namespace

int main()
{
    test_all_variants_are_registered();
    test_expected_variant_names();
    test_all_combinations_are_unique();
    test_variant_round_trip();
    test_variant_dimensions();
    test_specific_variants();
    test_invalid_variant_names();
    test_registry_lookup_is_deterministic();

    return 0;
}