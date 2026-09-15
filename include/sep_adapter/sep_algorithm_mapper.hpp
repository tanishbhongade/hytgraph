// include/sep_adapter/sep_algorithm_mapper.hpp
//
// Phase 14 — Data-movement bridge.
//
// Project-owned mapping between SEPAlgorithm and the vendored
// sepgraph::policy::AlgoType. This header forward-declares the
// vendored type so that no vendored symbol is exposed in a public
// project header. The implementation (sep_algorithm_mapper.cpp) is
// the ONLY translation unit that includes <framework/common.h> and
// <framework/hybrid_policy.h> for this mapping.
//
// Mapping:
//   SEPAlgorithm::IterativeScheme  <->  policy::AlgoType::ITERATIVE_SCHEME
//   SEPAlgorithm::TraversalScheme  <->  policy::AlgoType::TRAVERSAL_SCHEME
//
// The mapping is a bijection over the two AlgoType values.

#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace sepgraph
{
    namespace policy
    {
        enum class AlgoType;
    } // namespace policy
} // namespace sepgraph

namespace hytgraph
{
    namespace sep_adapter
    {

        // Project-owned algorithm classification, mirroring
        // sepgraph::policy::AlgoType without exposing the vendored enum.
        enum class SEPAlgorithm : std::uint8_t
        {
            IterativeScheme = 0, // PageRank, CC
            TraversalScheme = 1, // SSSP, BFS
        };

        constexpr std::uint8_t kSEPAlgorithmCount = 2;

        // String round-trip for configuration and test diagnostics.
        const char *to_string(SEPAlgorithm algo) noexcept;
        std::optional<SEPAlgorithm> from_string(const std::string &s) noexcept;

        // Validity check for untrusted input.
        constexpr bool is_valid_algorithm(SEPAlgorithm algo) noexcept
        {
            return algo == SEPAlgorithm::IterativeScheme ||
                   algo == SEPAlgorithm::TraversalScheme;
        }

        // Translate a project algorithm into the vendored AlgoType.
        // Returns std::nullopt if the algorithm is invalid.
        //
        // This function is declared here but defined only when
        // HYTGRAPH_WITH_SEP_GRAPH is enabled. In OFF builds a stub is
        // provided by sep_algorithm_mapper_stub.cpp.
        std::optional<sepgraph::policy::AlgoType>
        to_vendored(SEPAlgorithm algo) noexcept;

        // Translate a vendored AlgoType back into a project algorithm.
        // Returns std::nullopt if the vendored value has no preimage.
        std::optional<SEPAlgorithm>
        from_vendored(sepgraph::policy::AlgoType algo) noexcept;

    } // namespace sep_adapter
} // namespace hytgraph