// src/sep_adapter/sep_engine_factory.cpp
//
// Phase 13 placeholder translation unit for the SEP engine factory.
//
// The factory is header-only in Phase 13: every function in
// sep_engine_factory.hpp is either constexpr (the two predicates) or
// inline (the two factories and the null-driver factory). There is
// nothing to define in a .cpp file yet.
//
// Why this file exists at all:
//
//   - MASTER_PLAN.md §Phase 13 lists it as a deliverable. Keeping the
//     path stable now means the CMake source list (File 11) does not
//     need to change when a later phase adds real definitions.
//
//   - The .cpp includes the header it belongs to, which forces the
//     compiler to parse the header in isolation. If the header ever
//     acquires a dependency that needs a forward declaration or an
//     additional <...> include, this file fails to compile and we
//     find out immediately, not from a downstream TU.
//
// What later phases will add here:
//
//   - Phase 14 will introduce concrete app types (PageRank, SSSP,
//     BFS, CC). At that point this .cpp is a natural home for the
//     explicit template instantiations of the factory functions for
//     those concrete types, so that every TU that calls the factory
//     does not have to re-parse the adapter's template body.
//
//   - If Phase 14 introduces a registration mechanism so that app
//     types can be added without editing this .cpp, the registration
//     table also belongs here.
//
// This file must contain NO vendored includes and NO vendored symbols.
// When the factory begins returning real engine drivers, the actual
// engine construction will live inside SEPEngineAdapter (see
// sep_engine_adapter.cpp for the Phase 14 plan) and will not require
// this file to include framework/framework.cuh directly.

#include "sep_adapter/sep_engine_factory.hpp"

namespace hytgraph
{
    namespace sep_adapter
    {

        // Intentionally empty in Phase 13. See the file header for details.

    } // namespace sep_adapter
} // namespace hytgraph