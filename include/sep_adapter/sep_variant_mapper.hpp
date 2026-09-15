// include/sep_adapter/sep_variant_mapper.hpp
//
// Project-owned mapping between the project-owned SEPVariant enum and
// a project-owned descriptor of the three execution-parameter
// components that the vendored SEP-Graph engine expects.
//
// This header contains NO vendored includes. It forward-declares
// sepgraph::common::AlgoVariant so that the two internal translation
// functions (detail::to_vendored / detail::from_vendored) can be
// declared here without including any vendored header. The vendored
// header appears only in src/sep_adapter/sep_variant_mapper.cpp.
//
// ---------------------------------------------------------------------
// REVISION NOTE (supersedes the first delivery of this file):
//
//   The two detail:: declarations below lost their `noexcept`
//   specifier. Reason: the vendored AlgoVariant constructor is not
//   known to be noexcept, and returning std::optional is not
//   guaranteed noexcept either. Keeping `noexcept` on the
//   declarations would turn any thrown exception into
//   std::terminate, which is worse than letting it propagate.
//
//   The `bijective` claim in the section header below is also
//   over-stated; the actual mapping is injective (8 project values
//   into 11 vendored values; three vendored values have no preimage).
//   Documentation-only; no code change required here.
// ---------------------------------------------------------------------

#ifndef HYTGRAPH_SEP_ADAPTER_SEP_VARIANT_MAPPER_HPP
#define HYTGRAPH_SEP_ADAPTER_SEP_VARIANT_MAPPER_HPP

#include <cstdint>
#include <optional>
#include <string_view>

#include "sep_adapter/sep_variant.hpp"

// ---------------------------------------------------------------------------
// Forward declaration of the vendored variant type.
//
// We deliberately do NOT include <framework/common.h> here. The only
// translation unit permitted to include it is
// src/sep_adapter/sep_variant_mapper.cpp.
// ---------------------------------------------------------------------------
namespace sepgraph
{
    namespace common
    {
        class AlgoVariant;
    } // namespace common
} // namespace sepgraph

namespace hytgraph
{
    namespace sep_adapter
    {

        /// Project-owned descriptor of the three execution-parameter
        /// components that the vendored SEP-Graph engine selects among.
        struct AlgoVariantDescriptor
        {
            enum class Mode : std::uint8_t
            {
                Sync = 0,
                Async = 1,
            };
            enum class Direction : std::uint8_t
            {
                Push = 0,
                Pull = 1,
            };
            enum class Traversal : std::uint8_t
            {
                DataDriven = 0,
                TopologyDriven = 1,
            };

            Mode mode = Mode::Sync;
            Direction direction = Direction::Push;
            Traversal traversal = Traversal::DataDriven;
        };

        // ---------------------------------------------------------------------------
        // to_string overloads for logging and diagnostics.
        // ---------------------------------------------------------------------------

        [[nodiscard]] constexpr std::string_view
        to_string(AlgoVariantDescriptor::Mode m) noexcept
        {
            switch (m)
            {
            case AlgoVariantDescriptor::Mode::Sync:
                return "Sync";
            case AlgoVariantDescriptor::Mode::Async:
                return "Async";
            }
            return "Unknown";
        }

        [[nodiscard]] constexpr std::string_view
        to_string(AlgoVariantDescriptor::Direction d) noexcept
        {
            switch (d)
            {
            case AlgoVariantDescriptor::Direction::Push:
                return "Push";
            case AlgoVariantDescriptor::Direction::Pull:
                return "Pull";
            }
            return "Unknown";
        }

        [[nodiscard]] constexpr std::string_view
        to_string(AlgoVariantDescriptor::Traversal t) noexcept
        {
            switch (t)
            {
            case AlgoVariantDescriptor::Traversal::DataDriven:
                return "DataDriven";
            case AlgoVariantDescriptor::Traversal::TopologyDriven:
                return "TopologyDriven";
            }
            return "Unknown";
        }

        [[nodiscard]] constexpr std::string_view
        to_string(const AlgoVariantDescriptor &d) noexcept
        {
            using M = AlgoVariantDescriptor::Mode;
            using D = AlgoVariantDescriptor::Direction;
            using T = AlgoVariantDescriptor::Traversal;

            switch (d.mode)
            {
            case M::Sync:
                switch (d.direction)
                {
                case D::Push:
                    switch (d.traversal)
                    {
                    case T::DataDriven:
                        return "SYNC_PUSH_DD";
                    case T::TopologyDriven:
                        return "SYNC_PUSH_TD";
                    }
                    break;
                case D::Pull:
                    switch (d.traversal)
                    {
                    case T::DataDriven:
                        return "SYNC_PULL_DD";
                    case T::TopologyDriven:
                        return "SYNC_PULL_TD";
                    }
                    break;
                }
                break;
            case M::Async:
                switch (d.direction)
                {
                case D::Push:
                    switch (d.traversal)
                    {
                    case T::DataDriven:
                        return "ASYNC_PUSH_DD";
                    case T::TopologyDriven:
                        return "ASYNC_PUSH_TD";
                    }
                    break;
                case D::Pull:
                    switch (d.traversal)
                    {
                    case T::DataDriven:
                        return "ASYNC_PULL_DD";
                    case T::TopologyDriven:
                        return "ASYNC_PULL_TD";
                    }
                    break;
                }
                break;
            }
            return "INVALID";
        }

        // ---------------------------------------------------------------------------
        // Forward and reverse mapping (project-side, no vendored types).
        // ---------------------------------------------------------------------------

        [[nodiscard]] constexpr AlgoVariantDescriptor
        describe(SEPVariant v) noexcept
        {
            using M = AlgoVariantDescriptor::Mode;
            using D = AlgoVariantDescriptor::Direction;
            using T = AlgoVariantDescriptor::Traversal;

            switch (v)
            {
            case SEPVariant::SYNC_PUSH_DD:
                return {M::Sync, D::Push, T::DataDriven};
            case SEPVariant::SYNC_PULL_DD:
                return {M::Sync, D::Pull, T::DataDriven};
            case SEPVariant::SYNC_PUSH_TD:
                return {M::Sync, D::Push, T::TopologyDriven};
            case SEPVariant::SYNC_PULL_TD:
                return {M::Sync, D::Pull, T::TopologyDriven};
            case SEPVariant::ASYNC_PUSH_DD:
                return {M::Async, D::Push, T::DataDriven};
            case SEPVariant::ASYNC_PULL_DD:
                return {M::Async, D::Pull, T::DataDriven};
            case SEPVariant::ASYNC_PUSH_TD:
                return {M::Async, D::Push, T::TopologyDriven};
            case SEPVariant::ASYNC_PULL_TD:
                return {M::Async, D::Pull, T::TopologyDriven};
            }
            return {M::Sync, D::Push, T::DataDriven};
        }

        [[nodiscard]] inline std::optional<SEPVariant>
        from_descriptor(const AlgoVariantDescriptor &d) noexcept
        {
            using M = AlgoVariantDescriptor::Mode;
            using D = AlgoVariantDescriptor::Direction;
            using T = AlgoVariantDescriptor::Traversal;

            switch (d.mode)
            {
            case M::Sync:
                switch (d.direction)
                {
                case D::Push:
                    switch (d.traversal)
                    {
                    case T::DataDriven:
                        return SEPVariant::SYNC_PUSH_DD;
                    case T::TopologyDriven:
                        return SEPVariant::SYNC_PUSH_TD;
                    }
                    break;
                case D::Pull:
                    switch (d.traversal)
                    {
                    case T::DataDriven:
                        return SEPVariant::SYNC_PULL_DD;
                    case T::TopologyDriven:
                        return SEPVariant::SYNC_PULL_TD;
                    }
                    break;
                }
                break;
            case M::Async:
                switch (d.direction)
                {
                case D::Push:
                    switch (d.traversal)
                    {
                    case T::DataDriven:
                        return SEPVariant::ASYNC_PUSH_DD;
                    case T::TopologyDriven:
                        return SEPVariant::ASYNC_PUSH_TD;
                    }
                    break;
                case D::Pull:
                    switch (d.traversal)
                    {
                    case T::DataDriven:
                        return SEPVariant::ASYNC_PULL_DD;
                    case T::TopologyDriven:
                        return SEPVariant::ASYNC_PULL_TD;
                    }
                    break;
                }
                break;
            }
            return std::nullopt;
        }

        // ---------------------------------------------------------------------------
        // Internal vendored translation (for src/sep_adapter/ ONLY).
        // ---------------------------------------------------------------------------
        //
        // These two declarations have NO `noexcept`. The vendored AlgoVariant
        // constructor may throw, and std::optional construction is not
        // guaranteed noexcept. Callers that need strong exception guarantees
        // must wrap the calls themselves.
        //
        // These are NOT part of the module's supported API and may change
        // without notice. Only .cpp files under src/sep_adapter/ should call
        // them.

        namespace detail
        {

            [[nodiscard]] sepgraph::common::AlgoVariant
            to_vendored(const AlgoVariantDescriptor &d);

            [[nodiscard]] std::optional<AlgoVariantDescriptor>
            from_vendored(const sepgraph::common::AlgoVariant &v);

        } // namespace detail

    } // namespace sep_adapter
} // namespace hytgraph

#endif // HYTGRAPH_SEP_ADAPTER_SEP_VARIANT_MAPPER_HPP