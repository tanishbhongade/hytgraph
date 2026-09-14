#include <cassert>
#include <cstddef>
#include <type_traits>

#include "sep/sep_application_traits.hpp"

namespace
{

    using namespace hytgraph::sep;

    struct TestApplication
    {
        using value_type = double;
        using buffer_type = double;
        using weight_type = double;
    };

    struct FloatApplication
    {
        using value_type = float;
        using buffer_type = float;
        using weight_type = float;
    };

    struct InvalidApplication
    {
        using value_type = double;
    };

    void test_default_traits()
    {
        using traits = SEPApplicationTraits<TestApplication>;

        static_assert(!traits::requires_edge_weights);
        static_assert(traits::supports_activity_predicate);

        assert(!traits::requires_edge_weights);
        assert(traits::supports_activity_predicate);
    }

    void test_type_aliases()
    {
        static_assert(
            std::is_same_v<
                sep_application_value_t<TestApplication>,
                double>);

        static_assert(
            std::is_same_v<
                sep_application_buffer_t<TestApplication>,
                double>);

        static_assert(
            std::is_same_v<
                sep_application_weight_t<TestApplication>,
                double>);
    }

    void test_float_type_aliases()
    {
        static_assert(
            std::is_same_v<
                sep_application_value_t<FloatApplication>,
                float>);

        static_assert(
            std::is_same_v<
                sep_application_buffer_t<FloatApplication>,
                float>);

        static_assert(
            std::is_same_v<
                sep_application_weight_t<FloatApplication>,
                float>);
    }

    void test_compatible_application_detection()
    {
        static_assert(
            is_sep_application_compatible_v<TestApplication>);

        static_assert(
            is_sep_application_compatible_v<FloatApplication>);

        static_assert(
            !is_sep_application_compatible_v<InvalidApplication>);

        assert(is_sep_application_compatible_v<TestApplication>);
        assert(is_sep_application_compatible_v<FloatApplication>);
        assert(!is_sep_application_compatible_v<InvalidApplication>);
    }

    void test_sssp_traits()
    {
        using traits = SEPSSSPTraits<TestApplication>;

        static_assert(traits::requires_edge_weights);
        static_assert(traits::supports_activity_predicate);

        assert(traits::requires_edge_weights);
        assert(traits::supports_activity_predicate);
    }

    void test_pagerank_traits()
    {
        using traits = SEPPageRankTraits<TestApplication>;

        static_assert(!traits::requires_edge_weights);
        static_assert(traits::supports_activity_predicate);

        assert(!traits::requires_edge_weights);
        assert(traits::supports_activity_predicate);
    }

    void test_sssp_and_pagerank_requirements_differ()
    {
        static_assert(
            SEPSSSPTraits<TestApplication>::requires_edge_weights);

        static_assert(
            !SEPPageRankTraits<TestApplication>::requires_edge_weights);

        assert(
            SEPSSSPTraits<TestApplication>::requires_edge_weights !=
            SEPPageRankTraits<TestApplication>::requires_edge_weights);
    }

} // namespace

int main()
{
    test_default_traits();
    test_type_aliases();
    test_float_type_aliases();
    test_compatible_application_detection();
    test_sssp_traits();
    test_pagerank_traits();
    test_sssp_and_pagerank_requirements_differ();

    return 0;
}