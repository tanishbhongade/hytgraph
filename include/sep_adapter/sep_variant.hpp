// include/sep_adapter/sep_variant.hpp
//
// Project-owned SEP execution variant enumeration.
//
// This header is part of the sep_adapter module, which is the ONLY
// boundary between project code and the vendored SEP-Graph + Groute
// code. No vendored symbols appear here.
//
// The eight values correspond to the 2×2×2 combination of the three
// execution-parameter pairs that SEP-Graph can switch between:
//
//   Mode:       Sync | Async
//   Direction:  Push | Pull
//   Traversal:  DD (data-driven) | TD (topology-driven)
//
// See SEP-Graph (PPoPP'19) for the semantics of each pair.
//
// Deviation from MASTER_PLAN.md §Phase 13:
//   The plan assumes a to_string/from_string pair exists on
//   sepgraph::common::AlgoVariant and that round-tripping every value
//   is possible. The vendored AlgoVariant is a class with three
//   components, not an enum, and has no such pair. Therefore
//   to_string/from_string are PROJECT-OWNED utilities defined here
//   for the project-owned SEPVariant enum, not wrappers around
//   vendored functions.

#ifndef HYTGRAPH_SEP_ADAPTER_SEP_VARIANT_HPP
#define HYTGRAPH_SEP_ADAPTER_SEP_VARIANT_HPP

#include <cstdint>
#include <string_view>

namespace hytgraph
{
    namespace sep_adapter
    {

        /// Execution variant for the SEP-Graph engine.
        ///
        /// The eight values are the Cartesian product of:
        ///   - execution mode:    SYNC, ASYNC
        ///   - communication:     PUSH, PULL
        ///   - traversal:         DD, TD
        ///
        /// The naming convention is {MODE}_{DIRECTION}_{TRAVERSAL}.
        enum class SEPVariant : std::uint8_t
        {
            SYNC_PUSH_DD = 0,
            SYNC_PULL_DD = 1,
            SYNC_PUSH_TD = 2,
            SYNC_PULL_TD = 3,
            ASYNC_PUSH_DD = 4,
            ASYNC_PULL_DD = 5,
            ASYNC_PUSH_TD = 6,
            ASYNC_PULL_TD = 7,
        };

        /// Number of distinct values in SEPVariant.
        ///
        /// Kept as a constant so iteration and bounds-checking code does not
        /// rely on magic numbers.
        inline constexpr std::size_t kSEPVariantCount = 8;

        /// Canonical, stable string representation of a SEPVariant.
        ///
        /// The returned string_view points to a string literal and is valid
        /// for the lifetime of the program. An invalid variant returns
        /// "INVALID".
        [[nodiscard]] constexpr std::string_view to_string(SEPVariant v) noexcept
        {
            switch (v)
            {
            case SEPVariant::SYNC_PUSH_DD:
                return "SYNC_PUSH_DD";
            case SEPVariant::SYNC_PULL_DD:
                return "SYNC_PULL_DD";
            case SEPVariant::SYNC_PUSH_TD:
                return "SYNC_PUSH_TD";
            case SEPVariant::SYNC_PULL_TD:
                return "SYNC_PULL_TD";
            case SEPVariant::ASYNC_PUSH_DD:
                return "ASYNC_PUSH_DD";
            case SEPVariant::ASYNC_PULL_DD:
                return "ASYNC_PULL_DD";
            case SEPVariant::ASYNC_PUSH_TD:
                return "ASYNC_PUSH_TD";
            case SEPVariant::ASYNC_PULL_TD:
                return "ASYNC_PULL_TD";
            }
            return "INVALID";
        }

        /// Parse a string produced by to_string back into a SEPVariant.
        ///
        /// Returns true and writes the parsed variant on success. Returns
        /// false and leaves out untouched on failure (unknown string or
        /// invalid input).
        [[nodiscard]] constexpr bool from_string(std::string_view s,
                                                 SEPVariant &out) noexcept
        {
            if (s == "SYNC_PUSH_DD")
            {
                out = SEPVariant::SYNC_PUSH_DD;
                return true;
            }
            if (s == "SYNC_PULL_DD")
            {
                out = SEPVariant::SYNC_PULL_DD;
                return true;
            }
            if (s == "SYNC_PUSH_TD")
            {
                out = SEPVariant::SYNC_PUSH_TD;
                return true;
            }
            if (s == "SYNC_PULL_TD")
            {
                out = SEPVariant::SYNC_PULL_TD;
                return true;
            }
            if (s == "ASYNC_PUSH_DD")
            {
                out = SEPVariant::ASYNC_PUSH_DD;
                return true;
            }
            if (s == "ASYNC_PULL_DD")
            {
                out = SEPVariant::ASYNC_PULL_DD;
                return true;
            }
            if (s == "ASYNC_PUSH_TD")
            {
                out = SEPVariant::ASYNC_PUSH_TD;
                return true;
            }
            if (s == "ASYNC_PULL_TD")
            {
                out = SEPVariant::ASYNC_PULL_TD;
                return true;
            }
            return false;
        }

        /// Validate that an integer is a legal SEPVariant value.
        [[nodiscard]] constexpr bool is_valid_variant(std::uint8_t raw) noexcept
        {
            return raw < static_cast<std::uint8_t>(kSEPVariantCount);
        }

    } // namespace sep_adapter
} // namespace hytgraph

#endif // HYTGRAPH_SEP_ADAPTER_SEP_VARIANT_HPP