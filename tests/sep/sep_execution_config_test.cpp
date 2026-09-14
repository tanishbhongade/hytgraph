#include <array>
#include <cassert>
#include <string>

#include "sep/sep_execution_config.hpp"
#include "sep/sep_execution_variant.hpp"

namespace
{

    using namespace hytgraph::sep;

    void test_default_configuration()
    {
        const SEPExecutionConfig config{};

        assert(config.variant().to_string() == "SYNC_PUSH_DD");
    }

    void test_explicit_configuration()
    {
        const SEPExecutionVariant variant(
            ExecutionMode::ASYNC,
            MessagePassing::PULL,
            SchedulingMode::DATA_DRIVEN);

        const SEPExecutionConfig config(variant);

        assert(config.variant() == variant);
        assert(config.variant().to_string() == "ASYNC_PULL_DD");
    }

    void test_all_configurations_are_preserved()
    {
        const std::array<SEPExecutionVariant, 8> variants = {{SEPExecutionVariant(
                                                                  ExecutionMode::SYNC,
                                                                  MessagePassing::PUSH,
                                                                  SchedulingMode::DATA_DRIVEN),

                                                              SEPExecutionVariant(
                                                                  ExecutionMode::SYNC,
                                                                  MessagePassing::PUSH,
                                                                  SchedulingMode::TOPOLOGY_DRIVEN),

                                                              SEPExecutionVariant(
                                                                  ExecutionMode::SYNC,
                                                                  MessagePassing::PULL,
                                                                  SchedulingMode::DATA_DRIVEN),

                                                              SEPExecutionVariant(
                                                                  ExecutionMode::SYNC,
                                                                  MessagePassing::PULL,
                                                                  SchedulingMode::TOPOLOGY_DRIVEN),

                                                              SEPExecutionVariant(
                                                                  ExecutionMode::ASYNC,
                                                                  MessagePassing::PUSH,
                                                                  SchedulingMode::DATA_DRIVEN),

                                                              SEPExecutionVariant(
                                                                  ExecutionMode::ASYNC,
                                                                  MessagePassing::PUSH,
                                                                  SchedulingMode::TOPOLOGY_DRIVEN),

                                                              SEPExecutionVariant(
                                                                  ExecutionMode::ASYNC,
                                                                  MessagePassing::PULL,
                                                                  SchedulingMode::DATA_DRIVEN),

                                                              SEPExecutionVariant(
                                                                  ExecutionMode::ASYNC,
                                                                  MessagePassing::PULL,
                                                                  SchedulingMode::TOPOLOGY_DRIVEN)}};

        for (const auto &variant : variants)
        {
            const SEPExecutionConfig config(variant);

            assert(config.variant() == variant);
            assert(config.variant().to_string() ==
                   variant.to_string());
        }
    }

    void test_string_configuration()
    {
        const SEPExecutionConfig config =
            SEPExecutionConfig::from_string(
                "ASYNC_PULL_TD");

        assert(config.variant().execution_mode() ==
               ExecutionMode::ASYNC);

        assert(config.variant().message_passing() ==
               MessagePassing::PULL);

        assert(config.variant().scheduling_mode() ==
               SchedulingMode::TOPOLOGY_DRIVEN);

        assert(config.variant().to_string() ==
               "ASYNC_PULL_TD");
    }

    void test_string_round_trip()
    {
        const std::array<const char *, 8> names = {
            "SYNC_PUSH_DD",
            "SYNC_PUSH_TD",
            "SYNC_PULL_DD",
            "SYNC_PULL_TD",
            "ASYNC_PUSH_DD",
            "ASYNC_PUSH_TD",
            "ASYNC_PULL_DD",
            "ASYNC_PULL_TD"};

        for (const auto *name : names)
        {
            const auto config =
                SEPExecutionConfig::from_string(name);

            assert(config.variant().to_string() == name);
        }
    }

    void test_configuration_copy()
    {
        const SEPExecutionConfig original(
            SEPExecutionVariant(
                ExecutionMode::SYNC,
                MessagePassing::PULL,
                SchedulingMode::TOPOLOGY_DRIVEN));

        const SEPExecutionConfig copy(original);

        assert(copy.variant() == original.variant());
        assert(copy.variant().to_string() ==
               original.variant().to_string());
    }

    void test_invalid_configuration()
    {
        bool threw = false;

        try
        {
            (void)SEPExecutionConfig::from_string(
                "INVALID_VARIANT");
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
    test_default_configuration();
    test_explicit_configuration();
    test_all_configurations_are_preserved();
    test_string_configuration();
    test_string_round_trip();
    test_configuration_copy();
    test_invalid_configuration();

    return 0;
}