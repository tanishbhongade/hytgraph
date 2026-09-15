// include/sep_adapter/sep_engine_adapter.hpp
//
// Templated thin wrapper around the vendored SEP-Graph Engine.
//
// This header contains NO vendored symbols and NO vendored includes.
// It is safe to include from any project code and is buildable
// without CUDA.
//
// ---------------------------------------------------------------------
// MAJOR DEVIATIONS FROM MASTER_PLAN.md §Phase 13 (all documented in
// PROJECT_STATE.md; none are deviations from the paper):
//
//   D1. Template signature.
//       The plan sketched SEPEngineAdapter<TApp>. The vendored engine
//       is:
//         template<typename TValue, typename TBuffer, typename TWeight,
//                  template<typename, typename, typename, typename...>
//                    class TAppImpl,
//                  typename... UnusedData>
//         class Engine;
//       The adapter matches this signature so it can hold the real
//       engine in Phase 14 without an interface break.
//
//   D2. No engine construction in Phase 13.
//       Engine's constructor calls cudaGetDeviceProperties and
//       CreateStream. Phase 12's smoke test builds with plain g++;
//       constructing the engine would break that build. Phase 13
//       therefore only records the variant and algorithm. Actual
//       engine construction is deferred to Phase 14.
//
//   D3. execute_step() returns DEFERRED in Phase 13.
//       The vendored engine exposes no single-step API. Its only
//       public execution entry point is Start(), which runs the whole
//       algorithm to convergence in an internal loop. Phase 13 has no
//       data-movement machinery to feed Start() anyway. A real step
//       is introduced in Phase 14.
//
//   D4. native_engine_handle() returns nullptr in Phase 13.
//       Populated in Phase 14 once the engine is actually constructed.
//
//   D5. SEPAlgorithm enum introduced here.
//       The engine's constructor requires policy::AlgoType, a
//       vendored enum. SEPAlgorithm is a project-owned mirror. The
//       translation lives in Phase 14's bridge, not in this header.
//
//   D6. No .cpp in Phase 13.
//       All method bodies are trivial and defined inline below. The
//       plan's sep_engine_adapter.cpp placeholder is delivered as an
//       empty translation unit in File 8.
//
//   D7. AlgoVariant is not consumed here.
//       The engine does not take an AlgoVariant at construction.
//       sep_variant_mapper's translation is consumed by Phase 14's
//       bridge, not by this adapter.
// ---------------------------------------------------------------------

#ifndef HYTGRAPH_SEP_ADAPTER_SEP_ENGINE_ADAPTER_HPP
#define HYTGRAPH_SEP_ADAPTER_SEP_ENGINE_ADAPTER_HPP

#include <cstdint>
#include <memory>
#include <string_view>

#include "sep_adapter/sep_execution_driver.hpp"
#include "sep_adapter/sep_execution_result.hpp"
#include "sep_adapter/sep_variant.hpp"

namespace hytgraph
{
    namespace sep_adapter
    {

        /// Project-owned mirror of the two algorithm schemes SEP-Graph
        /// distinguishes.
        ///
        /// Vendored reality: the engine constructor takes policy::AlgoType,
        /// whose values are IterativeScheme and TraversalScheme (visible in
        /// Engine::PrintInfo()). SEPAlgorithm mirrors these; Phase 14's
        /// bridge translates between them.
        ///
        /// Naming: MASTER_PLAN.md §Phase 20 refers to this concept as
        /// "sep_adapter::Algorithm". We use SEPAlgorithm for symmetry with
        /// SEPVariant and to avoid a generic name in the public namespace.
        enum class SEPAlgorithm : std::uint8_t
        {
            IterativeScheme = 0, ///< iterative algorithms (PageRank, ...)
            TraversalScheme = 1, ///< traversal algorithms (BFS, SSSP, ...)
        };

        [[nodiscard]] constexpr std::string_view
        to_string(SEPAlgorithm a) noexcept
        {
            switch (a)
            {
            case SEPAlgorithm::IterativeScheme:
                return "IterativeScheme";
            case SEPAlgorithm::TraversalScheme:
                return "TraversalScheme";
            }
            return "Unknown";
        }

        /// Thin wrapper around the vendored SEP-Graph engine.
        ///
        /// Lifecycle (inherited from SEPExecutionDriver, see
        /// sep_execution_driver.hpp):
        ///
        ///   initialize()      -> OK in Phase 13 (records intent; does not
        ///                        construct the engine — see D2).
        ///   execute_step()    -> DEFERRED in Phase 13 (no single-step API in
        ///                        the vendored engine — see D3).
        ///   synchronize()     -> OK in Phase 13 (nothing is in flight).
        ///
        /// After Phase 14 these will become real operations.
        template <
            typename TValue,
            typename TBuffer,
            typename TWeight,
            template <typename, typename, typename, typename...> class TAppImpl,
            typename... UnusedData>
        class SEPEngineAdapter final : public SEPExecutionDriver
        {
        public:
            /// Construct the adapter for the given variant and algorithm.
            ///
            /// Does not touch CUDA, does not construct the vendored engine,
            /// does not allocate graph data. See D2.
            SEPEngineAdapter(SEPVariant variant, SEPAlgorithm algo)
                : variant_(variant),
                  algo_(algo),
                  initialized_(false),
                  engine_handle_(nullptr) {}

            ~SEPEngineAdapter() override = default;

            SEPEngineAdapter(const SEPEngineAdapter &) = delete;
            SEPEngineAdapter &operator=(const SEPEngineAdapter &) = delete;
            SEPEngineAdapter(SEPEngineAdapter &&) = delete;
            SEPEngineAdapter &operator=(SEPEngineAdapter &&) = delete;

            /// Phase 13: records that initialize() was called and returns OK.
            ///
            /// Does NOT construct the vendored Engine. See D2. Phase 14 will
            /// replace this body with an actual engine construction.
            ///
            /// Calling initialize() twice is allowed; the second call is a
            /// no-op and returns OK.
            SEPExecutionResult initialize() override
            {
                initialized_ = true;
                return make_ok(
                    "SEPEngineAdapter: initialized (Phase 13 placeholder; "
                    "engine construction deferred to Phase 14)");
            }

            /// Phase 13: returns DEFERRED, never performs work.
            ///
            /// Reason (D3): the vendored engine exposes no single-step API.
            /// Its only public execution entry point, Start(), runs the
            /// entire algorithm to convergence. Phase 13 also has no
            /// data-movement bridge to feed Start() with a graph. A real
            /// execute_step() arrives in Phase 14.
            ///
            /// Contract (mirrors NullSEPDriver so tests can be polymorphic):
            ///   - NEED_INIT if initialize() has not been called.
            ///   - DEFERRED otherwise.
            SEPExecutionResult execute_step() override
            {
                if (!initialized_)
                {
                    return make_need_init(
                        "SEPEngineAdapter: execute_step called before initialize");
                }
                return make_deferred(
                    "SEPEngineAdapter: execute_step is a no-op in Phase 13 "
                    "(no single-step API in the vendored engine; deferred to "
                    "Phase 14)");
            }

            /// Phase 13: returns OK.
            ///
            /// There is no asynchronous work to wait for. Once Phase 14
            /// introduces real engine calls this will become a real
            /// synchronization point.
            SEPExecutionResult synchronize() override
            {
                if (!initialized_)
                {
                    return make_need_init(
                        "SEPEngineAdapter: synchronize called before initialize");
                }
                return make_ok("SEPEngineAdapter: nothing to synchronize");
            }

            [[nodiscard]] SEPVariant variant() const noexcept override
            {
                return variant_;
            }

            [[nodiscard]] bool is_initialized() const noexcept override
            {
                return initialized_;
            }

            /// Returns the underlying vendored engine as an opaque handle.
            ///
            /// Phase 13: always nullptr (D4). Phase 14 will return the actual
            /// engine pointer after initialize() constructs it.
            ///
            /// Only code under src/sep_adapter/ may cast this to a vendored
            /// type. Anywhere else, treat it as opaque.
            [[nodiscard]] void *native_engine_handle() noexcept
            {
                return engine_handle_;
            }

            /// The algorithm this adapter was constructed with. Never fails.
            [[nodiscard]] SEPAlgorithm algorithm() const noexcept
            {
                return algo_;
            }

        private:
            SEPVariant variant_;
            SEPAlgorithm algo_;
            bool initialized_;
            void *engine_handle_; // nullptr in Phase 13 (D4)
        };

    } // namespace sep_adapter
} // namespace hytgraph

#endif // HYTGRAPH_SEP_ADAPTER_SEP_ENGINE_ADAPTER_HPP