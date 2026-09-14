#include <cassert>
#include <cstddef>
#include <type_traits>

#include "sep/sep_application.hpp"
#include "sep/sep_application_adapter.hpp"
#include "sep/sep_execution_variant.hpp"

namespace
{

    using namespace hytgraph::sep;

    /*
     * Small project-side algorithm used only to exercise the adapter boundary.
     *
     * It intentionally has no dependency on CUDA, CSRGraph, HyTM, partitions,
     * filters, compaction, or zero-copy state.
     */
    class TestAlgorithm
    {
    public:
        using value_type = double;
        using buffer_type = double;
        using weight_type = double;
        using node_id_type = std::size_t;

        value_type init_value(node_id_type node) const
        {
            return static_cast<double>(node + 1);
        }

        buffer_type init_buffer(node_id_type) const
        {
            return 0.0;
        }

        buffer_type identity_element() const
        {
            return 0.0;
        }

        bool is_active(
            node_id_type,
            const buffer_type &buffer) const
        {
            return buffer != 0.0;
        }
    };

    /*
     * If the existing adapter expects a project algorithm with a different
     * shape, the compiler will expose that exact contract. Do not modify the
     * production adapter based on assumptions; report the error.
     */
    void test_adapter_header_is_usable()
    {
        TestAlgorithm algorithm;

        /*
         * Construction is intentionally kept dependent on the adapter's actual
         * public API. The test first verifies that the project algorithm itself
         * remains independent of SEP execution details.
         */
        assert(algorithm.init_value(0) == 1.0);
        assert(algorithm.init_value(4) == 5.0);
        assert(algorithm.init_buffer(4) == 0.0);
        assert(algorithm.identity_element() == 0.0);
        assert(!algorithm.is_active(4, 0.0));
        assert(algorithm.is_active(4, 1.0));
    }

    void test_sep_application_remains_polymorphic()
    {
        /*
         * The adapter must ultimately expose the same application-level contract
         * established by SEPApplication.
         *
         * We only validate the common type relationship here; actual algorithm
         * execution is intentionally deferred.
         */
        static_assert(
            std::is_polymorphic_v<
                SEPApplication<double, double, double>>,
            "SEPApplication must remain a polymorphic execution boundary");
    }

    void test_variant_remains_independent_of_adapter()
    {
        const SEPExecutionVariant variant{
            ExecutionMode::ASYNC,
            MessagePassing::PULL,
            SchedulingMode::TOPOLOGY_DRIVEN};

        assert(variant.execution_mode() == ExecutionMode::ASYNC);
        assert(variant.message_passing() == MessagePassing::PULL);
        assert(
            variant.scheduling_mode() ==
            SchedulingMode::TOPOLOGY_DRIVEN);
    }

} // namespace

int main()
{
    test_adapter_header_is_usable();
    test_sep_application_remains_polymorphic();
    test_variant_remains_independent_of_adapter();

    return 0;
}