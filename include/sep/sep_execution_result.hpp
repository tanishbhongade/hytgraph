#ifndef HYTGRAPH_SEP_EXECUTION_RESULT_HPP
#define HYTGRAPH_SEP_EXECUTION_RESULT_HPP

#include <cstddef>

namespace hytgraph::sep
{

    /**
     * Status of a SEP execution operation.
     *
     * Phase 12 only defines the execution contract. Concrete CUDA/runtime
     * behavior is deferred to Phase 13.
     */
    enum class SEPExecutionStatus
    {
        SUCCESS = 0,
        NOT_INITIALIZED,
        INVALID_CONFIGURATION,
        INVALID_CONTEXT,
        NO_WORK,
        CONVERGED,
    };

    /**
     * Result returned by a SEP execution operation.
     *
     * This structure contains execution-level information only. It deliberately
     * does not expose CUDA streams, device pointers, transfer state, or
     * HyTGraph-specific state.
     */
    struct SEPExecutionResult
    {
        SEPExecutionStatus status{SEPExecutionStatus::SUCCESS};

        /**
         * Number of logical vertices processed by the operation.
         */
        std::size_t vertices_processed{0};

        /**
         * Number of logical messages/updates produced.
         */
        std::size_t updates_generated{0};

        /**
         * Whether the operation changed application state.
         */
        bool changed{false};

        /**
         * Whether the application reached its termination condition.
         */
        bool converged{false};

        constexpr bool succeeded() const noexcept
        {
            return status == SEPExecutionStatus::SUCCESS ||
                   status == SEPExecutionStatus::CONVERGED;
        }

        constexpr bool has_work() const noexcept
        {
            return vertices_processed != 0 ||
                   updates_generated != 0;
        }

        constexpr bool reached_convergence() const noexcept
        {
            return converged ||
                   status == SEPExecutionStatus::CONVERGED;
        }

        static constexpr SEPExecutionResult success(
            std::size_t vertices = 0,
            std::size_t updates = 0,
            bool state_changed = false) noexcept
        {
            return {
                SEPExecutionStatus::SUCCESS,
                vertices,
                updates,
                state_changed,
                false,
            };
        }

        static constexpr SEPExecutionResult no_work() noexcept
        {
            return {
                SEPExecutionStatus::NO_WORK,
                0,
                0,
                false,
                false,
            };
        }

        static constexpr SEPExecutionResult converged_result() noexcept
        {
            return {
                SEPExecutionStatus::CONVERGED,
                0,
                0,
                false,
                true,
            };
        }

        static constexpr SEPExecutionResult not_initialized() noexcept
        {
            return {
                SEPExecutionStatus::NOT_INITIALIZED,
                0,
                0,
                false,
                false,
            };
        }

        static constexpr SEPExecutionResult invalid_configuration() noexcept
        {
            return {
                SEPExecutionStatus::INVALID_CONFIGURATION,
                0,
                0,
                false,
                false,
            };
        }

        static constexpr SEPExecutionResult invalid_context() noexcept
        {
            return {
                SEPExecutionStatus::INVALID_CONTEXT,
                0,
                0,
                false,
                false,
            };
        }
    };

} // namespace hytgraph::sep

#endif // HYTGRAPH_SEP_EXECUTION_RESULT_HPP