// tests/sep_adapter_tests.cpp
//
// Phase 13 unit tests for the sep_adapter module.
//
// Scope:
//   - SEPVariant string round trip and validity.
//   - AlgoVariantDescriptor round trip and string consistency with
//     SEPVariant.
//   - SEPExecutionResult factory behavior.
//   - SEPExecutionState to_string.
//   - NullSEPDriver lifecycle contract.
//   - SEPEngineAdapter lifecycle contract.
//   - Factory behavior (make_null_driver, make_engine_driver,
//     make_engine_adapter).
//   - supports_variant / supports_algorithm predicates.
//   - Polymorphic lifecycle through SEPExecutionDriver*.
//
// NOT in scope (deliberately, in Phase 13):
//
//   - The vendored translation detail::to_vendored / from_vendored.
//     Calling these would require this TU to know the complete type
//     sepgraph::common::AlgoVariant, which is forward-declared only in
//     sep_variant_mapper.hpp. Instantiating that type in a project-side
//     test TU would violate the plan's boundary rule ("variant_mapper.
//     cpp is the only file that includes the vendored header"). The
//     vendored round trip is therefore deferred to Phase 14, where the
//     bridge can call these functions through a project-owned wrapper.
//
//   - Any kernel launch, data movement, or engine construction. Phase
//     13 has none of these.
//
// Build convention: plain C++17, no CUDA, no vendored includes. Links
// hytgraph_runtime (which carries the sep_adapter sources).

#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <string_view>

#include "sep_adapter/null_sep_driver.hpp"
#include "sep_adapter/sep_engine_adapter.hpp"
#include "sep_adapter/sep_engine_factory.hpp"
#include "sep_adapter/sep_execution_driver.hpp"
#include "sep_adapter/sep_execution_result.hpp"
#include "sep_adapter/sep_variant.hpp"
#include "sep_adapter/sep_variant_mapper.hpp"

using namespace hytgraph::sep_adapter;

// ---------------------------------------------------------------------------
// Minimal test harness
// ---------------------------------------------------------------------------
//
// The project already has a unit-test binary; this file is standalone and
// does not share its harness. A tiny macro-based harness keeps the file
// self-contained and dependency-free.
namespace
{

    int g_checks = 0;
    int g_failures = 0;

// CHECK is variadic so that expressions containing commas inside
// template argument lists can be passed without the preprocessor
// splitting them. CHECK_EQ takes exactly two arguments and is used
// only with comma-free expressions.
#define CHECK(...)                                          \
    do                                                      \
    {                                                       \
        ++g_checks;                                         \
        if (!(__VA_ARGS__))                                 \
        {                                                   \
            ++g_failures;                                   \
            std::fprintf(stderr, "FAIL %s:%d: %s\n",        \
                         __FILE__, __LINE__, #__VA_ARGS__); \
        }                                                   \
    } while (0)

#define CHECK_EQ(a, b)                                     \
    do                                                     \
    {                                                      \
        ++g_checks;                                        \
        if (!((a) == (b)))                                 \
        {                                                  \
            ++g_failures;                                  \
            std::fprintf(stderr, "FAIL %s:%d: %s == %s\n", \
                         __FILE__, __LINE__, #a, #b);      \
        }                                                  \
    } while (0)

    // ---------------------------------------------------------------------------
    // DummyApp: the template-template argument for SEPEngineAdapter.
    //
    // The adapter does not instantiate TAppImpl in Phase 13 (engine
    // construction is deferred to Phase 14), so an empty body is
    // sufficient. The parameter list must match the adapter's template-
    // template parameter declaration:
    //     template <typename, typename, typename, typename...> class TAppImpl
    // ---------------------------------------------------------------------------
    template <typename TValue, typename TBuffer, typename TWeight, typename...>
    struct DummyApp
    {
    };

    using IntAdapter = SEPEngineAdapter<int, int, int, DummyApp>;

    // ---------------------------------------------------------------------------
    // SEPVariant
    // ---------------------------------------------------------------------------

    void test_sep_variant_strings()
    {
        for (std::uint8_t i = 0; i < kSEPVariantCount; ++i)
        {
            auto v = static_cast<SEPVariant>(i);
            const auto s = to_string(v);
            CHECK(!s.empty());
            CHECK(s != "INVALID");

            SEPVariant back{};
            CHECK(from_string(s, back));
            CHECK(back == v);
        }

        // Unknown strings must be rejected without touching `out`.
        SEPVariant out = SEPVariant::SYNC_PUSH_DD;
        CHECK(!from_string("NOT_A_VARIANT", out));
        CHECK(out == SEPVariant::SYNC_PUSH_DD); // unchanged

        CHECK(!from_string("", out));
        CHECK(!from_string("sync_push_dd", out));  // case-sensitive
        CHECK(!from_string("SYNC_PUSH_DD ", out)); // trailing space
    }

    void test_sep_variant_validity()
    {
        CHECK(is_valid_variant(0));
        CHECK(is_valid_variant(1));
        CHECK(is_valid_variant(6));
        CHECK(is_valid_variant(7));
        CHECK(!is_valid_variant(8));
        CHECK(!is_valid_variant(16));
        CHECK(!is_valid_variant(255));

        CHECK_EQ(kSEPVariantCount, std::size_t{8});
    }

    // ---------------------------------------------------------------------------
    // AlgoVariantDescriptor
    // ---------------------------------------------------------------------------

    void test_descriptor_roundtrip()
    {
        for (std::uint8_t i = 0; i < kSEPVariantCount; ++i)
        {
            const auto v = static_cast<SEPVariant>(i);
            const auto d = describe(v);
            const auto back = from_descriptor(d);
            CHECK(back.has_value());
            CHECK(*back == v);
        }
    }

    void test_descriptor_to_string_matches_variant()
    {
        // The composed descriptor string must equal the SEPVariant string
        // for the same value. This cross-checks that the two independent
        // tables in File 1 and File 4a agree.
        for (std::uint8_t i = 0; i < kSEPVariantCount; ++i)
        {
            const auto v = static_cast<SEPVariant>(i);
            const auto d = describe(v);
            const std::string_view ds = to_string(d);
            const std::string_view vs = to_string(v);
            CHECK_EQ(ds, vs);
        }
    }

    void test_descriptor_component_strings()
    {
        using M = AlgoVariantDescriptor::Mode;
        using D = AlgoVariantDescriptor::Direction;
        using T = AlgoVariantDescriptor::Traversal;

        CHECK_EQ(std::string_view(to_string(M::Sync)), std::string_view("Sync"));
        CHECK_EQ(std::string_view(to_string(M::Async)), std::string_view("Async"));
        CHECK_EQ(std::string_view(to_string(D::Push)), std::string_view("Push"));
        CHECK_EQ(std::string_view(to_string(D::Pull)), std::string_view("Pull"));
        CHECK_EQ(std::string_view(to_string(T::DataDriven)),
                 std::string_view("DataDriven"));
        CHECK_EQ(std::string_view(to_string(T::TopologyDriven)),
                 std::string_view("TopologyDriven"));
    }

    // ---------------------------------------------------------------------------
    // SEPExecutionResult
    // ---------------------------------------------------------------------------

    void test_execution_state_to_string()
    {
        CHECK_EQ(std::string_view(to_string(SEPExecutionState::OK)),
                 std::string_view("OK"));
        CHECK_EQ(std::string_view(to_string(SEPExecutionState::NEED_INIT)),
                 std::string_view("NEED_INIT"));
        CHECK_EQ(std::string_view(to_string(SEPExecutionState::INVALID_CONFIG)),
                 std::string_view("INVALID_CONFIG"));
        CHECK_EQ(std::string_view(to_string(SEPExecutionState::DEFERRED)),
                 std::string_view("DEFERRED"));
        CHECK_EQ(std::string_view(to_string(SEPExecutionState::FAILED)),
                 std::string_view("FAILED"));
    }

    void test_execution_result_factories()
    {
        // Default-constructed result is OK.
        {
            SEPExecutionResult r;
            CHECK(r.state == SEPExecutionState::OK);
            CHECK(r.is_ok());
            CHECK(!r.is_error());
            CHECK(r.message.empty());
        }

        // make_ok
        {
            auto r = make_ok();
            CHECK(r.state == SEPExecutionState::OK);
            CHECK(r.is_ok());
        }
        {
            auto r = make_ok("all good");
            CHECK(r.is_ok());
            CHECK_EQ(r.message, std::string("all good"));
        }

        // make_need_init
        {
            auto r = make_need_init();
            CHECK(r.state == SEPExecutionState::NEED_INIT);
            CHECK(r.is_error());
            CHECK(!r.is_ok());
        }

        // make_invalid_config
        {
            auto r = make_invalid_config("bad config");
            CHECK(r.state == SEPExecutionState::INVALID_CONFIG);
            CHECK(r.is_error());
            CHECK_EQ(r.message, std::string("bad config"));
        }

        // make_deferred
        {
            auto r = make_deferred();
            CHECK(r.state == SEPExecutionState::DEFERRED);
            CHECK(r.is_error()); // DEFERRED is not OK
        }

        // make_failed
        {
            auto r = make_failed("exploded");
            CHECK(r.state == SEPExecutionState::FAILED);
            CHECK(r.is_error());
            CHECK_EQ(r.message, std::string("exploded"));
        }
    }

    // ---------------------------------------------------------------------------
    // NullSEPDriver
    // ---------------------------------------------------------------------------

    void test_null_driver_lifecycle()
    {
        for (std::uint8_t i = 0; i < kSEPVariantCount; ++i)
        {
            const auto v = static_cast<SEPVariant>(i);
            NullSEPDriver d(v);

            CHECK(!d.is_initialized());
            CHECK(d.variant() == v);

            // Before initialize: NEED_INIT.
            CHECK(d.execute_step().state == SEPExecutionState::NEED_INIT);
            CHECK(d.synchronize().state == SEPExecutionState::NEED_INIT);

            // initialize returns DEFERRED, marks initialized.
            CHECK(d.initialize().state == SEPExecutionState::DEFERRED);
            CHECK(d.is_initialized());

            // After initialize: DEFERRED for both.
            CHECK(d.execute_step().state == SEPExecutionState::DEFERRED);
            CHECK(d.synchronize().state == SEPExecutionState::DEFERRED);

            // Second initialize is idempotent.
            CHECK(d.initialize().state == SEPExecutionState::DEFERRED);
            CHECK(d.is_initialized());
        }
    }

    void test_null_driver_via_factory()
    {
        for (std::uint8_t i = 0; i < kSEPVariantCount; ++i)
        {
            const auto v = static_cast<SEPVariant>(i);
            auto d = make_null_driver(v);

            CHECK(d != nullptr);
            CHECK(d->variant() == v);
            CHECK(!d->is_initialized());

            CHECK(d->initialize().state == SEPExecutionState::DEFERRED);
            CHECK(d->is_initialized());
            CHECK(d->execute_step().state == SEPExecutionState::DEFERRED);

            // Concrete type is reachable.
            CHECK(dynamic_cast<NullSEPDriver *>(d.get()) != nullptr);
        }
    }

    // ---------------------------------------------------------------------------
    // SEPEngineAdapter
    // ---------------------------------------------------------------------------

    void test_engine_adapter_lifecycle()
    {
        for (std::uint8_t i = 0; i < kSEPVariantCount; ++i)
        {
            const auto v = static_cast<SEPVariant>(i);
            for (auto algo : {SEPAlgorithm::IterativeScheme,
                              SEPAlgorithm::TraversalScheme})
            {
                auto a = make_engine_adapter<int, int, int, DummyApp>(v, algo);

                CHECK(a != nullptr);
                CHECK(a->variant() == v);
                CHECK(a->algorithm() == algo);
                CHECK(!a->is_initialized());

                // Phase 13: no engine is constructed; handle is nullptr.
                CHECK(a->native_engine_handle() == nullptr);

                CHECK(a->execute_step().state == SEPExecutionState::NEED_INIT);
                CHECK(a->synchronize().state == SEPExecutionState::NEED_INIT);

                CHECK(a->initialize().state == SEPExecutionState::OK);
                CHECK(a->is_initialized());

                CHECK(a->execute_step().state == SEPExecutionState::DEFERRED);
                CHECK(a->synchronize().state == SEPExecutionState::OK);

                // Second initialize is idempotent.
                CHECK(a->initialize().state == SEPExecutionState::OK);
                CHECK(a->is_initialized());
            }
        }
    }

    void test_engine_adapter_via_factory()
    {
        // make_engine_driver returns a SEPEngineAdapter in ON builds and a
        // NullSEPDriver in OFF builds. This test asserts the gating.
        for (std::uint8_t i = 0; i < kSEPVariantCount; ++i)
        {
            const auto v = static_cast<SEPVariant>(i);
            auto d = make_engine_driver<int, int, int, DummyApp>(
                v, SEPAlgorithm::IterativeScheme);

            CHECK(d != nullptr);
            CHECK(d->variant() == v);
            CHECK(!d->is_initialized());

#if defined(HYTGRAPH_WITH_SEP_GRAPH) && (HYTGRAPH_WITH_SEP_GRAPH == 1)
            CHECK(dynamic_cast<IntAdapter *>(d.get()) != nullptr);
            CHECK(dynamic_cast<NullSEPDriver *>(d.get()) == nullptr);
#else
            CHECK(dynamic_cast<IntAdapter *>(d.get()) == nullptr);
            CHECK(dynamic_cast<NullSEPDriver *>(d.get()) != nullptr);
#endif
        }
    }

    // ---------------------------------------------------------------------------
    // Predicates
    // ---------------------------------------------------------------------------

    void test_supports_predicates()
    {
        for (std::uint8_t i = 0; i < kSEPVariantCount; ++i)
        {
            CHECK(supports_variant(static_cast<SEPVariant>(i)));
        }
        CHECK(supports_algorithm(SEPAlgorithm::IterativeScheme));
        CHECK(supports_algorithm(SEPAlgorithm::TraversalScheme));
    }

    // ---------------------------------------------------------------------------
    // Polymorphism through SEPExecutionDriver*
    // ---------------------------------------------------------------------------

    void test_polymorphic_lifecycle()
    {
        // A null driver and an engine driver must answer the lifecycle
        // questions identically when accessed through the base pointer.
        // Only the exact return state of initialize/synchronize may differ
        // (DEFERRED vs OK); the presence/absence of NEED_INIT must agree.
        for (std::uint8_t i = 0; i < kSEPVariantCount; ++i)
        {
            const auto v = static_cast<SEPVariant>(i);

            std::unique_ptr<SEPExecutionDriver> a = make_null_driver(v);
            std::unique_ptr<SEPExecutionDriver> b =
                make_engine_driver<int, int, int, DummyApp>(
                    v, SEPAlgorithm::IterativeScheme);

            for (SEPExecutionDriver *d : {a.get(), b.get()})
            {
                CHECK(!d->is_initialized());
                CHECK(d->variant() == v);

                CHECK(d->execute_step().state == SEPExecutionState::NEED_INIT);
                CHECK(d->synchronize().state == SEPExecutionState::NEED_INIT);

                auto init_state = d->initialize().state;
                CHECK(init_state == SEPExecutionState::OK ||
                      init_state == SEPExecutionState::DEFERRED);
                CHECK(d->is_initialized());

                auto step_state = d->execute_step().state;
                CHECK(step_state == SEPExecutionState::OK ||
                      step_state == SEPExecutionState::DEFERRED);

                auto sync_state = d->synchronize().state;
                CHECK(sync_state == SEPExecutionState::OK ||
                      sync_state == SEPExecutionState::DEFERRED);
            }
        }
    }

} // namespace

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    test_sep_variant_strings();
    test_sep_variant_validity();
    test_descriptor_roundtrip();
    test_descriptor_to_string_matches_variant();
    test_descriptor_component_strings();
    test_execution_state_to_string();
    test_execution_result_factories();
    test_null_driver_lifecycle();
    test_null_driver_via_factory();
    test_engine_adapter_lifecycle();
    test_engine_adapter_via_factory();
    test_supports_predicates();
    test_polymorphic_lifecycle();

    std::printf("sep_adapter_tests: %d checks, %d failures\n",
                g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}