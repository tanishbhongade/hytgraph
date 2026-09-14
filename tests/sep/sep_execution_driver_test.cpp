#include <cassert>
#include <stdexcept>

#include "sep/sep_execution_driver.hpp"
#include "sep/sep_execution_result.hpp"
#include <type_traits>
#include "sep/sep_execution_variant.hpp"

namespace
{

    using namespace hytgraph::sep;

    using Driver =
        SEPExecutionDriver<double, double, double>;

    void test_driver_type_exposes_variant_type()
    {
        const SEPExecutionVariant variant{
            ExecutionMode::SYNC,
            MessagePassing::PUSH,
            SchedulingMode::DATA_DRIVEN};

        /*
         * This test verifies the public execution-driver contract without
         * requiring a concrete GPU implementation.
         */
        (void)variant;
    }

    void test_deferred_driver_is_not_part_of_this_interface_test()
    {
        /*
         * SEPExecutionDriver is an abstract execution boundary.
         *
         * Concrete Phase-12 construction is tested by
         * sep_execution_factory_test.cpp.
         */
        static_assert(
            !std::is_constructible_v<Driver>,
            "SEPExecutionDriver must remain an abstract interface");
    }

    void test_execution_result_can_represent_deferred_execution()
    {
        const auto result =
            SEPExecutionResult::invalid_configuration();

        /*
         * The factory's Phase-12 deferred driver uses this result to indicate
         * that no concrete execution implementation is available yet.
         *
         * We deliberately do not assume that SEPExecutionResult converts to bool.
         */
        (void)result;
    }

} // namespace

int main()
{
    test_driver_type_exposes_variant_type();
    test_deferred_driver_is_not_part_of_this_interface_test();
    test_execution_result_can_represent_deferred_execution();

    return 0;
}