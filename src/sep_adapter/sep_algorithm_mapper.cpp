// src/sep_adapter/sep_algorithm_mapper.cpp
//
// Phase 14 — Data-movement bridge.
//
// Project-owned mapping between SEPAlgorithm and the vendored
// sepgraph::policy::AlgoType.
//
// This translation unit is the ONLY place (besides
// sep_engine_adapter_impl.cu) that includes
// <framework/hybrid_policy.h>.
//
// Build policy: compiled only when HYTGRAPH_WITH_SEP_GRAPH=ON.
// In OFF builds, the two symbols declared in
// include/sep_adapter/sep_algorithm_mapper.hpp are declared but
// undefined. Nothing in Phase 14 references them in OFF builds.
// Phase 20 will add stubs (see PROJECT_STATE.md Known Issue #25).

#include "sep_adapter/sep_algorithm_mapper.hpp"

#include <framework/hybrid_policy.h>

namespace hytgraph
{
    namespace sep_adapter
    {

        // ---------------------------------------------------------------------------
        // String round-trip
        // ---------------------------------------------------------------------------

        const char *to_string(SEPAlgorithm algo) noexcept
        {
            switch (algo)
            {
            case SEPAlgorithm::IterativeScheme:
                return "IterativeScheme";
            case SEPAlgorithm::TraversalScheme:
                return "TraversalScheme";
            }
            return "Unknown";
        }

        std::optional<SEPAlgorithm> from_string(const std::string &s) noexcept
        {
            if (s == "IterativeScheme")
                return SEPAlgorithm::IterativeScheme;
            if (s == "TraversalScheme")
                return SEPAlgorithm::TraversalScheme;
            return std::nullopt;
        }

        // ---------------------------------------------------------------------------
        // Project -> vendored
        // ---------------------------------------------------------------------------
        //
        // The mapping is a bijection over the two AlgoType enumerators.
        // Invalid SEPAlgorithm values return std::nullopt.

        std::optional<sepgraph::policy::AlgoType>
        to_vendored(SEPAlgorithm algo) noexcept
        {
            switch (algo)
            {
            case SEPAlgorithm::IterativeScheme:
                return sepgraph::policy::AlgoType::ITERATIVE_SCHEME;
            case SEPAlgorithm::TraversalScheme:
                return sepgraph::policy::AlgoType::TRAVERSAL_SCHEME;
            }
            return std::nullopt;
        }

        // ---------------------------------------------------------------------------
        // Vendored -> project
        // ---------------------------------------------------------------------------
        //
        // AlgoType carries exactly two enumerators, both with preimages.
        // The mapping is a bijection, unlike SEPVariant <-> AlgoVariant.

        std::optional<SEPAlgorithm>
        from_vendored(sepgraph::policy::AlgoType algo) noexcept
        {
            switch (algo)
            {
            case sepgraph::policy::AlgoType::ITERATIVE_SCHEME:
                return SEPAlgorithm::IterativeScheme;
            case sepgraph::policy::AlgoType::TRAVERSAL_SCHEME:
                return SEPAlgorithm::TraversalScheme;
            }
            return std::nullopt;
        }

    } // namespace sep_adapter
} // namespace hytgraph