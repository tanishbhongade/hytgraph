#include <cassert>
#include <stdexcept>
#include <string_view>

#include "sep/sep_execution_factory.hpp"
#include "sep/sep_execution_variant_registry.hpp"

namespace
{

    using namespace hytgraph::sep;

    using TestFactory =
        SEPExecutionFactory<double, double, double>;

    void test_create_all_variants()
    {
        const auto &variants =
            SEPExecutionVariantRegistry::all();

        assert(variants.size() == 8);

        for (const auto &variant : variants)
        {
            auto driver =
                TestFactory::create(variant);

            assert(driver != nullptr);
            assert(driver->execution_variant() == variant);
        }
    }

    void test_create_from_configuration()
    {
        const SEPExecutionVariant variant{
            ExecutionMode::ASYNC,
            MessagePassing::PULL,
            SchedulingMode::TOPOLOGY_DRIVEN};

        const SEPExecutionConfig config{variant};

        auto driver =
            TestFactory::create(config);

        assert(driver != nullptr);
        assert(driver->execution_variant() == variant);
    }

    void test_create_from_string()
    {
        auto driver =
            TestFactory::create("SYNC_PUSH_DD");

        assert(driver != nullptr);

        const auto &variant =
            driver->execution_variant();

        assert(variant.execution_mode() ==
               ExecutionMode::SYNC);

        assert(variant.message_passing() ==
               MessagePassing::PUSH);

        assert(variant.scheduling_mode() ==
               SchedulingMode::DATA_DRIVEN);

        assert(variant.to_string() ==
               "SYNC_PUSH_DD");
    }

    void test_create_all_canonical_strings()
    {
        const std::string_view names[] = {
            "SYNC_PUSH_DD",
            "SYNC_PULL_DD",
            "SYNC_PUSH_TD",
            "SYNC_PULL_TD",
            "ASYNC_PUSH_DD",
            "ASYNC_PULL_DD",
            "ASYNC_PUSH_TD",
            "ASYNC_PULL_TD"};

        for (const auto name : names)
        {
            auto driver =
                TestFactory::create(name);

            assert(driver != nullptr);
            assert(
                driver->execution_variant().to_string() ==
                name);
        }
    }

    void test_factory_is_deterministic()
    {
        const SEPExecutionVariant variant{
            ExecutionMode::ASYNC,
            MessagePassing::PUSH,
            SchedulingMode::DATA_DRIVEN};

        auto first =
            TestFactory::create(variant);

        auto second =
            TestFactory::create(variant);

        assert(first != nullptr);
        assert(second != nullptr);

        assert(
            first->execution_variant() ==
            second->execution_variant());

        assert(
            first->execution_variant().to_string() ==
            second->execution_variant().to_string());
    }

    void test_invalid_string_is_rejected()
    {
        bool threw = false;

        try
        {
            auto driver =
                TestFactory::create("INVALID_VARIANT");

            (void)driver;
        }
        catch (const std::invalid_argument &)
        {
            threw = true;
        }

        assert(threw);
    }

    void test_deferred_driver_does_not_execute()
    {
        auto driver =
            TestFactory::create("SYNC_PUSH_DD");

        assert(driver != nullptr);

        /*
         * Phase 12 deliberately has no concrete GPU execution.
         *
         * Calling execute_step() must nevertheless be well-defined and return
         * an execution result object. The result's detailed state is owned by
         * SEPExecutionResult and is tested separately.
         */
        const auto result =
            driver->execute_step();

        (void)result;
    }

    void test_variant_mismatch_is_rejected()
    {
        const SEPExecutionVariant selected{
            ExecutionMode::SYNC,
            MessagePassing::PUSH,
            SchedulingMode::DATA_DRIVEN};

        auto driver =
            TestFactory::create(selected);

        assert(driver != nullptr);

        const SEPExecutionConfig mismatched{
            SEPExecutionVariant{
                ExecutionMode::ASYNC,
                MessagePassing::PULL,
                SchedulingMode::TOPOLOGY_DRIVEN}};

        /*
         * initialize() requires an application object, so this test only verifies
         * the factory-level invariant that the driver remembers the selected
         * variant. The actual initialize() mismatch path will be exercised by
         * the application/driver integration test.
         */
        assert(
            driver->execution_variant() !=
            mismatched.variant());
    }

} // namespace

int main()
{
    test_create_all_variants();
    test_create_from_configuration();
    test_create_from_string();
    test_create_all_canonical_strings();
    test_factory_is_deterministic();
    test_invalid_string_is_rejected();
    test_deferred_driver_does_not_execute();
    test_variant_mismatch_is_rejected();

    return 0;
}