// include/sep_adapter/null_sep_driver.hpp
//
// Header-only null implementation of SEPExecutionDriver.
//
// Purpose:
//   - Exercise the SEPExecutionDriver abstraction without touching
//     the vendored SEP-Graph engine.
//   - Serve as the driver returned by the factory when
//     HYTGRAPH_WITH_SEP_GRAPH is OFF, or when the caller explicitly
//     requests a driver that does nothing.
//   - Enable Phase 13's adapter tests to run in environments where
//     the vendored code cannot be built.
//
// This header contains NO vendored symbols. It depends only on
// sep_execution_driver.hpp, sep_execution_result.hpp, and
// sep_variant.hpp.
//
// Behavior contract (documented; tests in File 12 assert it):
//
//   initialize():
//       Returns DEFERRED. Sets the internal "initialized" flag to
//       true so that subsequent execute_step()/synchronize() calls
//       return DEFERRED rather than NEED_INIT. The flag is set even
//       though the return state is DEFERRED, because "initialize()
//       has been called and the driver is now ready to accept the
//       remaining lifecycle calls" is true; DEFERRED here means "no
//       real engine work was performed," not "the call was ignored."
//
//   execute_step():
//       Returns NEED_INIT if initialize() has not been called.
//       Returns DEFERRED otherwise. Does nothing.
//
//   synchronize():
//       Returns NEED_INIT if initialize() has not been called.
//       Returns DEFERRED otherwise. Does nothing.
//
//   variant():
//       Returns the variant passed at construction. noexcept.
//
//   is_initialized():
//       True iff initialize() has been called on this instance.
//       noexcept.
//
// Deviation from MASTER_PLAN.md §Phase 13:
//   The plan says only "null driver returning DEFERRED." It does not
//   specify the pre-initialization behavior of execute_step() /
//   synchronize(), nor the semantics of is_initialized() after a
//   DEFERRED initialize(). The choices above keep the driver
//   interface contract (File 3) coherent: execute_step() before
//   initialize() is NEED_INIT for every driver, including the null
//   driver, so callers cannot distinguish null drivers from real
//   drivers by that check.

#ifndef HYTGRAPH_SEP_ADAPTER_NULL_SEP_DRIVER_HPP
#define HYTGRAPH_SEP_ADAPTER_NULL_SEP_DRIVER_HPP

#include "sep_adapter/sep_execution_driver.hpp"
#include "sep_adapter/sep_execution_result.hpp"
#include "sep_adapter/sep_variant.hpp"

namespace hytgraph
{
    namespace sep_adapter
    {

        /// Null SEP execution driver. Every operation is a no-op.
        ///
        /// Not thread-safe in Phase 13 (matching the base interface
        /// contract). The only mutable state is the initialized flag.
        class NullSEPDriver final : public SEPExecutionDriver
        {
        public:
            /// Construct a null driver for the given variant.
            ///
            /// The variant is stored verbatim and returned by variant(). It
            /// does not affect behavior — the null driver treats all variants
            /// identically.
            explicit NullSEPDriver(SEPVariant variant) noexcept
                : variant_(variant), initialized_(false) {}

            ~NullSEPDriver() override = default;

            NullSEPDriver(const NullSEPDriver &) = delete;
            NullSEPDriver &operator=(const NullSEPDriver &) = delete;
            NullSEPDriver(NullSEPDriver &&) = delete;
            NullSEPDriver &operator=(NullSEPDriver &&) = delete;

            /// Returns DEFERRED and marks the driver as initialized.
            ///
            /// Calling initialize() twice is allowed: the second call also
            /// returns DEFERRED and leaves the driver initialized.
            SEPExecutionResult initialize() override
            {
                initialized_ = true;
                return make_deferred(
                    "NullSEPDriver: initialize is a no-op");
            }

            /// Returns NEED_INIT before initialize(), DEFERRED after.
            SEPExecutionResult execute_step() override
            {
                if (!initialized_)
                {
                    return make_need_init(
                        "NullSEPDriver: execute_step called before initialize");
                }
                return make_deferred(
                    "NullSEPDriver: execute_step is a no-op");
            }

            /// Returns NEED_INIT before initialize(), DEFERRED after.
            SEPExecutionResult synchronize() override
            {
                if (!initialized_)
                {
                    return make_need_init(
                        "NullSEPDriver: synchronize called before initialize");
                }
                return make_deferred(
                    "NullSEPDriver: synchronize is a no-op");
            }

            [[nodiscard]] SEPVariant variant() const noexcept override
            {
                return variant_;
            }

            [[nodiscard]] bool is_initialized() const noexcept override
            {
                return initialized_;
            }

        private:
            SEPVariant variant_;
            bool initialized_;
        };

    } // namespace sep_adapter
} // namespace hytgraph

#endif // HYTGRAPH_SEP_ADAPTER_NULL_SEP_DRIVER_HPP