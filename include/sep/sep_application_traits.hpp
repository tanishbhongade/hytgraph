#ifndef HYTGRAPH_SEP_APPLICATION_TRAITS_HPP
#define HYTGRAPH_SEP_APPLICATION_TRAITS_HPP

#include <cstddef>
#include <type_traits>

namespace hytgraph::sep
{

    /**
     * Execution-level traits for SEP applications.
     *
     * These traits describe data requirements rather than algorithm behavior.
     *
     * They are intentionally independent from SEPApplication itself so that
     * existing PageRank and SSSP implementations do not need to be rewritten
     * merely to participate in the Phase 12 execution foundation.
     *
     * Default:
     *   - no edge weights required;
     *   - activity predicate supported;
     *   - no special value/buffer relationship assumed.
     */
    template <typename Application>
    struct SEPApplicationTraits
    {
        static constexpr bool requires_edge_weights = false;

        static constexpr bool supports_activity_predicate = true;
    };

    /**
     * Convenience aliases for extracting the data types exposed by a
     * SEPApplication-compatible implementation.
     */
    template <typename Application>
    using sep_application_value_t =
        typename Application::value_type;

    template <typename Application>
    using sep_application_buffer_t =
        typename Application::buffer_type;

    template <typename Application>
    using sep_application_weight_t =
        typename Application::weight_type;

    /**
     * Compile-time detection for the core SEP application type interface.
     *
     * This does not require the application to inherit from SEPApplication.
     * It only checks whether the expected data-type aliases exist.
     */
    template <typename Application, typename = void>
    struct is_sep_application_compatible
        : std::false_type
    {
    };

    template <typename Application>
    struct is_sep_application_compatible<
        Application,
        std::void_t<
            typename Application::value_type,
            typename Application::buffer_type,
            typename Application::weight_type>>
        : std::true_type
    {
    };

    template <typename Application>
    inline constexpr bool is_sep_application_compatible_v =
        is_sep_application_compatible<Application>::value;

    /**
     * SSSP-specific trait specialization.
     *
     * SSSP requires edge weights during message accumulation.
     *
     * This specialization only describes the requirement; it does not implement
     * weighted relaxation.
     */
    template <typename Application>
    struct SEPSSSPTraits
    {
        static constexpr bool requires_edge_weights = true;

        static constexpr bool supports_activity_predicate = true;
    };

    /**
     * PageRank-specific trait specialization.
     *
     * PageRank's SEP message accumulation can operate without an explicit
     * per-edge weight supplied by the SEP application interface.
     */
    template <typename Application>
    struct SEPPageRankTraits
    {
        static constexpr bool requires_edge_weights = false;

        static constexpr bool supports_activity_predicate = true;
    };

} // namespace hytgraph::sep

#endif // HYTGRAPH_SEP_APPLICATION_TRAITS_HPP