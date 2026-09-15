// include/sep_adapter/sep_execution_result.hpp
//
// Project-owned result type returned by every SEPExecutionDriver
// operation (initialize, execute_step, synchronize).
//
// This header is part of the sep_adapter module and contains no
// vendored symbols.
//
// Design notes:
//
//   - State is a plain enum class rather than a variant because every
//     call site needs to branch on the state and read the message.
//     A variant would add boilerplate for no benefit.
//
//   - message is a std::string rather than std::string_view because
//     the message may be built at runtime (e.g., "engine construction
//     failed: <reason>"). Ownership must travel with the result.
//
//   - The default state is OK with an empty message. This matches the
//     plan's declaration:
//         State state = State::OK;
//         std::string message;
//     Callers that construct a default result are explicitly saying
//     "nothing has gone wrong yet."
//
//   - Convenience factory functions (ok, need_init, invalid_config,
//     deferred, failed) are provided so call sites do not have to
//     spell out the State enumerator. They are inline and constexpr-
//     friendly where possible.
//
// Deviation from MASTER_PLAN.md §Phase 13:
//   The plan lists the five State values but does not specify whether
//   SEPExecutionResult should carry a code, a message, or both. We
//   carry a message only. If a numeric code is later required, it can
//   be added without breaking the interface because the struct is
//   used by value and by const reference, never as an aggregate
//   initializer in a positional way.

#ifndef HYTGRAPH_SEP_ADAPTER_SEP_EXECUTION_RESULT_HPP
#define HYTGRAPH_SEP_ADAPTER_SEP_EXECUTION_RESULT_HPP

#include <string>
#include <string_view>
#include <utility>

namespace hytgraph
{
    namespace sep_adapter
    {

        /// Outcome of a single SEPExecutionDriver operation.
        ///
        /// Semantics of each state:
        ///
        ///   OK              — The operation completed successfully.
        ///
        ///   NEED_INIT       — The driver has not been initialized yet.
        ///                     Callers should call initialize() and retry.
        ///                     Returned by execute_step() and synchronize()
        ///                     on an uninitialized driver.
        ///
        ///   INVALID_CONFIG  — The configuration supplied at construction is
        ///                     not usable (e.g., an unsupported variant, a
        ///                     null app, or a factory argument mismatch).
        ///                     Retrying will not help; the caller must fix
        ///                     the configuration.
        ///
        ///   DEFERRED        — The operation is a no-op in this build or in
        ///                     this driver implementation. The null driver
        ///                     returns DEFERRED from every method so the
        ///                     abstraction can be exercised without the
        ///                     vendored engine. Phase 13 uses DEFERRED as
        ///                     the "not yet wired to data movement" signal.
        ///
        ///   FAILED          — The operation attempted real work and failed.
        ///                     The message field describes the failure. This
        ///                     includes vendored-engine construction
        ///                     failures and any future kernel failures.
        enum class SEPExecutionState
        {
            OK = 0,
            NEED_INIT,
            INVALID_CONFIG,
            DEFERRED,
            FAILED,
        };

        /// Canonical, stable string representation of a SEPExecutionState.
        ///
        /// The returned string_view points to a string literal and is valid
        /// for the lifetime of the program. An unknown value returns
        /// "UNKNOWN".
        [[nodiscard]] constexpr std::string_view to_string(SEPExecutionState s) noexcept
        {
            switch (s)
            {
            case SEPExecutionState::OK:
                return "OK";
            case SEPExecutionState::NEED_INIT:
                return "NEED_INIT";
            case SEPExecutionState::INVALID_CONFIG:
                return "INVALID_CONFIG";
            case SEPExecutionState::DEFERRED:
                return "DEFERRED";
            case SEPExecutionState::FAILED:
                return "FAILED";
            }
            return "UNKNOWN";
        }

        /// Result of a SEPExecutionDriver operation.
        struct SEPExecutionResult
        {
            SEPExecutionState state = SEPExecutionState::OK;
            std::string message;

            /// Default-construct an OK result with no message.
            SEPExecutionResult() = default;

            /// Construct a result with an explicit state and message.
            SEPExecutionResult(SEPExecutionState s, std::string msg)
                : state(s), message(std::move(msg)) {}

            /// True when the operation completed successfully.
            [[nodiscard]] bool is_ok() const noexcept
            {
                return state == SEPExecutionState::OK;
            }

            /// True when the operation did not complete successfully.
            /// This includes DEFERRED; callers that treat DEFERRED as a
            /// success must check state explicitly.
            [[nodiscard]] bool is_error() const noexcept
            {
                return state != SEPExecutionState::OK;
            }
        };

        // ---------------------------------------------------------------------------
        // Convenience factories
        // ---------------------------------------------------------------------------
        //
        // These exist so call sites read naturally:
        //
        //     return SEPExecutionResult::ok();
        //     return SEPExecutionResult::need_init("call initialize() first");
        //     return SEPExecutionResult::invalid_config("null app");
        //
        // They are free functions rather than static members so the struct
        // remains an aggregate-friendly plain data type and so the factories
        // can be found via ADL on SEPExecutionResult.

        [[nodiscard]] inline SEPExecutionResult
        make_ok(std::string msg = {})
        {
            return SEPExecutionResult(SEPExecutionState::OK, std::move(msg));
        }

        [[nodiscard]] inline SEPExecutionResult
        make_need_init(std::string msg = "driver not initialized")
        {
            return SEPExecutionResult(SEPExecutionState::NEED_INIT, std::move(msg));
        }

        [[nodiscard]] inline SEPExecutionResult
        make_invalid_config(std::string msg)
        {
            return SEPExecutionResult(SEPExecutionState::INVALID_CONFIG, std::move(msg));
        }

        [[nodiscard]] inline SEPExecutionResult
        make_deferred(std::string msg = "operation deferred")
        {
            return SEPExecutionResult(SEPExecutionState::DEFERRED, std::move(msg));
        }

        [[nodiscard]] inline SEPExecutionResult
        make_failed(std::string msg)
        {
            return SEPExecutionResult(SEPExecutionState::FAILED, std::move(msg));
        }

    } // namespace sep_adapter
} // namespace hytgraph

#endif // HYTGRAPH_SEP_ADAPTER_SEP_EXECUTION_RESULT_HPP