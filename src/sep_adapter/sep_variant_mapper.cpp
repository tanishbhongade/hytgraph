// src/sep_adapter/sep_variant_mapper.cpp
//
// Vendored translation implementation for sep_variant_mapper.
//
// This is the ONLY translation unit in the project (excluding the
// Phase 12 smoke test) that includes any header from
// third_party/hytgraph_sep/. It includes <framework/common.h>,
// translated through the <tuple>/<string>/<cassert> shim established
// in Phase 12.
//
// ---------------------------------------------------------------------
// REVISION NOTE (supersedes the first delivery of this file):
//
//   The first version of this file assumed AlgoVariant was a value
//   type constructed from three enumerators: (Model, MsgPassing,
//   Scheduling). framework/framework.cuh shows this is wrong. In the
//   vendored code AlgoVariant is a class exposing NAMED STATIC
//   CONSTANTS:
//
//       AlgoVariant::SYNC_PUSH_DD, SYNC_PULL_DD, SYNC_PUSH_TD,
//       SYNC_PULL_TD, ASYNC_PUSH_DD, ASYNC_PULL_DD, ASYNC_PUSH_TD,
//       ASYNC_PULL_TD,
//       AlgoVariant::Exp_Filter, AlgoVariant::Zero_Copy,
//       AlgoVariant::Exp_Compaction.
//
//   Confirmed from framework.cuh (direct usage):
//       AlgoVariant::ASYNC_PUSH_DD
//       AlgoVariant::Exp_Filter
//       AlgoVariant::Zero_Copy
//       AlgoVariant::Exp_Compaction
//       AlgoVariant::ToString()
//       operator== (used as `algo_variant[i] == AlgoVariant::Exp_Filter`)
//       default-constructible and copy-assignable
//
//   Inferred by symmetry with ASYNC_PUSH_DD and the paper's parameter
//   names (marked VENDOR-CHECK: below):
//       AlgoVariant::SYNC_PUSH_DD, SYNC_PULL_DD, SYNC_PUSH_TD,
//       SYNC_PULL_TD, ASYNC_PULL_DD, ASYNC_PUSH_TD, ASYNC_PULL_TD
//
//   Mapping is INJECTIVE, not bijective:
//     - our 8 SEPVariant values map to 8 distinct AlgoVariant values,
//     - AlgoVariant additionally carries Exp_Filter, Zero_Copy,
//       Exp_Compaction, which have NO preimage in SEPVariant.
//
//   from_vendored() returns std::nullopt for those three transfer-
//   engine values. to_vendored() never produces them.
// ---------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Build-environment shims for the vendored header.
//
// common.h uses std::tie (<tuple>), std::string (<string>), and
// assert (<cassert>) without including the corresponding headers.
// Supplying them here keeps the vendored tree unmodified.
// ---------------------------------------------------------------------------
#include <cassert>
#include <string>
#include <tuple>

// Vendored header — the ONLY one in this translation unit.
#include <framework/common.h>

#include "sep_adapter/sep_variant_mapper.hpp"

#include <type_traits>

namespace hytgraph
{
    namespace sep_adapter
    {
        namespace detail
        {

            // ---------------------------------------------------------------------------
            // to_vendored
            // ---------------------------------------------------------------------------
            //
            // Return the vendored named constant for the given descriptor.
            //
            // Every project-owned descriptor has exactly one vendored counterpart.
            // The three transfer-engine variants (Exp_Filter, Zero_Copy,
            // Exp_Compaction) are NOT produced here; they are not part of the
            // execution-parameter space that SEPVariant describes.
            //
            // VENDOR-CHECK: only ASYNC_PUSH_DD is confirmed by framework.cuh.
            // The other seven names are inferred from the paper's parameter
            // naming. If the vendored header spells any of them differently
            // (e.g., lowercase, or with a different separator), the compiler
            // will report an unknown member; paste the error and I will fix in
            // place.

            sepgraph::common::AlgoVariant
            to_vendored(const AlgoVariantDescriptor &d)
            {
                using AV = sepgraph::common::AlgoVariant;
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
                            return AV::SYNC_PUSH_DD; // VENDOR-CHECK
                        case T::TopologyDriven:
                            return AV::SYNC_PUSH_TD; // VENDOR-CHECK
                        }
                        break;
                    case D::Pull:
                        switch (d.traversal)
                        {
                        case T::DataDriven:
                            return AV::SYNC_PULL_DD; // VENDOR-CHECK
                        case T::TopologyDriven:
                            return AV::SYNC_PULL_TD; // VENDOR-CHECK
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
                            return AV::ASYNC_PUSH_DD; // CONFIRMED
                        case T::TopologyDriven:
                            return AV::ASYNC_PUSH_TD; // VENDOR-CHECK
                        }
                        break;
                    case D::Pull:
                        switch (d.traversal)
                        {
                        case T::DataDriven:
                            return AV::ASYNC_PULL_DD; // VENDOR-CHECK
                        case T::TopologyDriven:
                            return AV::ASYNC_PULL_TD; // VENDOR-CHECK
                        }
                        break;
                    }
                    break;
                }
                return AV::SYNC_PUSH_DD; // VENDOR-CHECK
            }

            // ---------------------------------------------------------------------------
            // from_vendored
            // ---------------------------------------------------------------------------
            //
            // Return the descriptor for a vendored AlgoVariant, or std::nullopt if
            // the vendored value is one of the transfer-engine variants that has
            // no counterpart in our execution-parameter space.

            std::optional<AlgoVariantDescriptor>
            from_vendored(const sepgraph::common::AlgoVariant &v)
            {
                using AV = sepgraph::common::AlgoVariant;
                using M = AlgoVariantDescriptor::Mode;
                using D = AlgoVariantDescriptor::Direction;
                using T = AlgoVariantDescriptor::Traversal;

                if (v == AV::SYNC_PUSH_DD) // VENDOR-CHECK
                    return AlgoVariantDescriptor{M::Sync, D::Push, T::DataDriven};
                if (v == AV::SYNC_PULL_DD) // VENDOR-CHECK
                    return AlgoVariantDescriptor{M::Sync, D::Pull, T::DataDriven};
                if (v == AV::SYNC_PUSH_TD) // VENDOR-CHECK
                    return AlgoVariantDescriptor{M::Sync, D::Push, T::TopologyDriven};
                if (v == AV::SYNC_PULL_TD) // VENDOR-CHECK
                    return AlgoVariantDescriptor{M::Sync, D::Pull, T::TopologyDriven};

                if (v == AV::ASYNC_PUSH_DD) // CONFIRMED
                    return AlgoVariantDescriptor{M::Async, D::Push, T::DataDriven};
                if (v == AV::ASYNC_PULL_DD) // VENDOR-CHECK
                    return AlgoVariantDescriptor{M::Async, D::Pull, T::DataDriven};
                if (v == AV::ASYNC_PUSH_TD) // VENDOR-CHECK
                    return AlgoVariantDescriptor{M::Async, D::Push, T::TopologyDriven};
                if (v == AV::ASYNC_PULL_TD) // VENDOR-CHECK
                    return AlgoVariantDescriptor{M::Async, D::Pull, T::TopologyDriven};

                return std::nullopt;
            }

        } // namespace detail
    } // namespace sep_adapter
} // namespace hytgraph