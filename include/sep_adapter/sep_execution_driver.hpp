// include/sep_adapter/sep_execution_driver.hpp
//
// Abstract base class for every SEP execution driver.
//
// This header is part of the sep_adapter module and contains no
// vendored symbols. All concrete drivers (null driver in Phase 13,
// vendored-engine adapter in Phase 13, and any future drivers) derive
// from SEPExecutionDriver.
//
// Design notes:
//
//   - The interface is intentionally minimal: initialize, step,
//     synchronize, plus two read-only observers. This matches the
//     plan's declared surface exactly. Adding methods later is
//     source-compatible for callers because SEPExecutionDriver is
//     used polymorphically.
//
//   - initialize() is separated from construction so that a driver
//     can be constructed without touching the vendored engine, then
//     initialized at a controlled point (e.g., after CUDA context
//     setup, or after the caller has confirmed the graph is loaded).
//
//   - execute_step() runs exactly one SEP iteration. It does not
//     loop; the caller owns the iteration schedule. This is required
//     by the plan ("Toy app executes one SEP step through the
//     adapter") and by Phase 14+ where the bridge must interleave
//     steps with data movement.
//
//   - synchronize() blocks until any in-flight work on the driver
//     has completed. In Phase 13 no driver launches asynchronous
//     work, so synchronize() is a no-op that still returns OK. This
//     keeps the interface stable for Phase 19 (multi-stream).
//
//   - variant() and is_initialized() are noexcept because they are
//     pure observers that must never fail. Returning a value rather
//     than a result keeps logging and dispatch code simple.
//
//   - The destructor is virtual and defaulted. Concrete drivers own
//     whatever resources the vendored engine requires, and their
//     pimpl destructors handle cleanup. A virtual default destructor
//     is correct here because nothing in the base needs to run.
//
// Deviation from MASTER_PLAN.md §Phase 13:
//   The plan's sketch uses SEPExecutionResult::State. We use the
//   top-level SEPExecutionState introduced in File 2. This is a
//   syntactic difference only; the five state values are identical.
//   A nested alias (using State = SEPExecutionState;) can be added to
//   SEPExecutionResult if desired, without changing this file.

#ifndef HYTGRAPH_SEP_ADAPTER_SEP_EXECUTION_DRIVER_HPP
#define HYTGRAPH_SEP_ADAPTER_SEP_EXECUTION_DRIVER_HPP

#include "sep_adapter/sep_execution_result.hpp"
#include "sep_adapter/sep_variant.hpp"

namespace hytgraph
{
    namespace sep_adapter
    {

        /// Abstract driver for the SEP-Graph execution engine.
        ///
        /// Lifecycle contract:
        ///
        ///   1. The caller constructs a concrete driver (via the factory, via
        ///      the null driver, or directly).
        ///   2. The caller calls initialize(). It may be called more than
        ///      once; a second call on an already-initialized driver must
        ///      return OK without re-constructing the engine. Implementations
        ///      that cannot satisfy this must document the deviation.
        ///   3. The caller calls execute_step() zero or more times. Calling
        ///      it before initialize() must return NEED_INIT.
        ///   4. The caller optionally calls synchronize() to wait for
        ///      in-flight work.
        ///   5. The caller destroys the driver. Destruction of an
        ///      uninitialized driver is safe.
        ///
        /// Thread-safety:
        ///   Phase 13 drivers are NOT required to be thread-safe. The
        ///   multi-stream runtime (Phase 19) will introduce the necessary
        ///   synchronization. Do not call two methods on the same driver
        ///   concurrently in Phase 13.
        class SEPExecutionDriver
        {
        public:
            virtual ~SEPExecutionDriver() = default;

            SEPExecutionDriver(const SEPExecutionDriver &) = delete;
            SEPExecutionDriver &operator=(const SEPExecutionDriver &) = delete;
            SEPExecutionDriver(SEPExecutionDriver &&) = delete;
            SEPExecutionDriver &operator=(SEPExecutionDriver &&) = delete;

            /// Construct or attach to the underlying engine.
            ///
            /// Returns:
            ///   OK              — the driver is ready for execute_step().
            ///   INVALID_CONFIG  — the driver cannot be initialized with the
            ///                     configuration it was constructed with.
            ///   FAILED          — engine construction failed. The message
            ///                     field describes the failure.
            ///
            /// Must not return NEED_INIT or DEFERRED.
            virtual SEPExecutionResult initialize() = 0;

            /// Execute exactly one SEP iteration.
            ///
            /// Returns:
            ///   OK              — the step completed.
            ///   NEED_INIT       — initialize() has not been called yet.
            ///   DEFERRED        — the driver is a no-op (null driver).
            ///   FAILED          — the step attempted work and failed.
            ///
            /// Must not return INVALID_CONFIG; that state is reserved for
            /// construction/initialization failures.
            virtual SEPExecutionResult execute_step() = 0;

            /// Wait for any in-flight work to complete.
            ///
            /// Returns:
            ///   OK              — all work has completed.
            ///   NEED_INIT       — initialize() has not been called yet.
            ///   DEFERRED        — the driver is a no-op (null driver).
            ///   FAILED          — the wait itself failed.
            ///
            /// Must not return INVALID_CONFIG.
            virtual SEPExecutionResult synchronize() = 0;

            /// The variant this driver was constructed with. Never fails.
            [[nodiscard]] virtual SEPVariant variant() const noexcept = 0;

            /// True if initialize() has completed successfully and the
            /// driver is ready for execute_step(). Never fails.
            [[nodiscard]] virtual bool is_initialized() const noexcept = 0;

        protected:
            /// Protected default constructor. Concrete drivers are
            /// constructed through their own public constructors (or through
            /// the factory). This prevents accidental slicing through the
            /// base.
            SEPExecutionDriver() = default;
        };

    } // namespace sep_adapter
} // namespace hytgraph

#endif // HYTGRAPH_SEP_ADAPTER_SEP_EXECUTION_DRIVER_HPP