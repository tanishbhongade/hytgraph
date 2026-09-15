// include/sep_adapter/sep_engine_factory.hpp
//
// Project-owned factory for SEP execution drivers.
//
// This header contains NO vendored symbols and NO vendored includes.
// It is safe to include from any project code and is buildable
// without CUDA.
//
// ---------------------------------------------------------------------
// MAJOR DEVIATIONS FROM MASTER_PLAN.md §Phase 13 (all documented in
// PROJECT_STATE.md; none are deviations from the paper):
//
//   F1. No (algorithm, variant)-pair-driven non-templated factory.
//       The plan sketched a factory that "produces a driver per
//       (algorithm, variant) pair." The vendored engine is templated
//       on (TValue, TBuffer, TWeight, TAppImpl, UnusedData...), and
//       TAppImpl is a template-template parameter that the factory
//       cannot select on its own. A non-templated factory returning
//       SEPEngineAdapter<...> is therefore impossible. We expose:
//
//         - make_null_driver(variant)                 — always available
//         - make_engine_driver<...>(variant, algo)    — templated
//         - supports_variant / supports_algorithm     — predicates
//
//       This is a build-boundary consequence of the vendored
//       signature, not a semantic change to what the factory does.
//
//   F2. HYTGRAPH_WITH_SEP_GRAPH gating.
//       When HYTGRAPH_WITH_SEP_GRAPH is OFF, make_engine_driver
//       returns a NullSEPDriver instead of a SEPEngineAdapter. This
//       honors the plan's "Support HYTGRAPH_WITH_SEP_GRAPH=OFF build
//       with the null driver" requirement at the factory level rather
//       than at every call site.
//
//   F3. Phase 13 make_engine_driver is fully functional.
//       In Phase 13 the adapter does not construct the vendored
//       engine (see sep_engine_adapter.hpp D2). make_engine_driver
//       therefore works even in plain-g++ builds without CUDA, and
//       even when the vendored headers are not on the include path
//       (see F2). When Phase 14 begins constructing the engine, the
//       gating in F2 will start mattering for real.
// ---------------------------------------------------------------------

#ifndef HYTGRAPH_SEP_ADAPTER_SEP_ENGINE_FACTORY_HPP
#define HYTGRAPH_SEP_ADAPTER_SEP_ENGINE_FACTORY_HPP

#include <memory>

#include "sep_adapter/null_sep_driver.hpp"
#include "sep_adapter/sep_engine_adapter.hpp"
#include "sep_adapter/sep_execution_driver.hpp"
#include "sep_adapter/sep_execution_result.hpp"
#include "sep_adapter/sep_variant.hpp"

namespace hytgraph
{
    namespace sep_adapter
    {

        // ---------------------------------------------------------------------------
        // Predicates
        // ---------------------------------------------------------------------------

        /// True for every currently defined SEPVariant value.
        ///
        /// Exists so call sites and tests do not need to enumerate the eight
        /// values themselves. It also gives us a single place to tighten the
        /// predicate if the engine ever stops supporting one of the eight
        /// combinations (e.g., if a future SEP-Graph revision removes a mode).
        [[nodiscard]] constexpr bool
        supports_variant(SEPVariant v) noexcept
        {
            return is_valid_variant(static_cast<std::uint8_t>(v));
        }

        /// True for every currently defined SEPAlgorithm value.
        [[nodiscard]] constexpr bool
        supports_algorithm(SEPAlgorithm a) noexcept
        {
            switch (a)
            {
            case SEPAlgorithm::IterativeScheme:
                return true;
            case SEPAlgorithm::TraversalScheme:
                return true;
            }
            return false;
        }

        // ---------------------------------------------------------------------------
        // Null driver factory
        // ---------------------------------------------------------------------------
        //
        // Always available, never gated. Callers that explicitly want a no-op
        // driver should use this rather than relying on the HYTGRAPH_WITH_SEP_
        // GRAPH macro. Tests use it as the reference implementation of the
        // SEPExecutionDriver lifecycle contract.

        [[nodiscard]] inline std::unique_ptr<SEPExecutionDriver>
        make_null_driver(SEPVariant variant)
        {
            return std::unique_ptr<SEPExecutionDriver>(
                new NullSEPDriver(variant));
        }

        // ---------------------------------------------------------------------------
        // Engine driver factory
        // ---------------------------------------------------------------------------
        //
        // Templated because the vendored engine requires the full type
        // signature (TValue, TBuffer, TWeight, TAppImpl, UnusedData...) at
        // compile time. The caller supplies the app type; the factory
        // supplies the variant and algorithm.
        //
        // Returns a unique_ptr<SEPExecutionDriver>. Callers that need the
        // concrete type (e.g., to call native_engine_handle()) can
        // dynamic_cast or use make_engine_adapter below.

        template <
            typename TValue,
            typename TBuffer,
            typename TWeight,
            template <typename, typename, typename, typename...> class TAppImpl,
            typename... UnusedData>
        [[nodiscard]] std::unique_ptr<SEPExecutionDriver>
        make_engine_driver(SEPVariant variant, SEPAlgorithm algo)
        {
#if defined(HYTGRAPH_WITH_SEP_GRAPH) && (HYTGRAPH_WITH_SEP_GRAPH == 0)
            // Vendored engine disabled at build time. Fall back to the null
            // driver so callers see a coherent lifecycle (NEED_INIT before
            // initialize(), DEFERRED after) without needing to know that the
            // engine is unavailable. See F2.
            (void)algo; // unused in the OFF build
            return make_null_driver(variant);
#else
            using Adapter = SEPEngineAdapter<
                TValue, TBuffer, TWeight, TAppImpl, UnusedData...>;
            return std::unique_ptr<SEPExecutionDriver>(
                new Adapter(variant, algo));
#endif
        }

        /// Typed variant of make_engine_driver that returns the concrete
        /// adapter type. Use this when the caller needs
        /// native_engine_handle(), algorithm(), or the templated identity.
        ///
        /// When HYTGRAPH_WITH_SEP_GRAPH is OFF, this still returns a
        /// SEPEngineAdapter (not a NullSEPDriver), because the concrete type
        /// is part of the signature. The adapter is fully functional in
        /// Phase 13 even without the vendored engine (see F3), so this is
        /// safe. Phase 14 will re-evaluate.
        template <
            typename TValue,
            typename TBuffer,
            typename TWeight,
            template <typename, typename, typename, typename...> class TAppImpl,
            typename... UnusedData>
        [[nodiscard]] std::unique_ptr<SEPEngineAdapter<
            TValue, TBuffer, TWeight, TAppImpl, UnusedData...>>
        make_engine_adapter(SEPVariant variant, SEPAlgorithm algo)
        {
            using Adapter = SEPEngineAdapter<
                TValue, TBuffer, TWeight, TAppImpl, UnusedData...>;
            return std::unique_ptr<Adapter>(new Adapter(variant, algo));
        }

    } // namespace sep_adapter
} // namespace hytgraph

#endif // HYTGRAPH_SEP_ADAPTER_SEP_ENGINE_FACTORY_HPP