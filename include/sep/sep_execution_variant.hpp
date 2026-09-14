#ifndef HYTGRAPH_SEP_EXECUTION_VARIANT_HPP
#define HYTGRAPH_SEP_EXECUTION_VARIANT_HPP

#include <array>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>

namespace hytgraph::sep
{

    /**
     * SEP execution-model dimension.
     *
     * Source/adaptation:
     *   SEP-Graph common.h -> Model
     *
     * The paper defines Sync as iteration-separated execution with a barrier,
     * while Async permits updates to become visible without an explicit
     * inter-iteration barrier.
     */
    enum class ExecutionMode
    {
        SYNC = 0,
        ASYNC = 1,
    };

    /**
     * SEP message-passing dimension.
     *
     * Source/adaptation:
     *   SEP-Graph common.h -> MsgPassing
     *
     * PUSH: an updated source actively sends an update to destinations.
     * PULL: a destination reads updates from its sources.
     */
    enum class MessagePassing
    {
        PUSH = 0,
        PULL = 1,
    };

    /**
     * SEP traversal/scheduling dimension.
     *
     * Source/adaptation:
     *   SEP-Graph common.h -> Scheduling
     *
     * DATA_DRIVEN traverses active work only.
     * TOPOLOGY_DRIVEN traverses the topology/work domain regardless of activity.
     */
    enum class SchedulingMode
    {
        DATA_DRIVEN = 0,
        TOPOLOGY_DRIVEN = 1,
    };

    /**
     * Stable representation of one SEP execution variant.
     *
     * SEP-Graph represents a variant as the Cartesian product of:
     *   Model × MsgPassing × Scheduling
     *
     * This project-local type intentionally contains only execution policy
     * information. It does not own graph data, activity state, partitions,
     * transfer engines, work queues, CUDA streams, or cache state.
     *
     * That separation is intentional for Phase 12:
     *   graph/activity/tasks/partitions remain owned by the existing project;
     *   SEP owns only execution policy representation.
     */
    class SEPExecutionVariant
    {
    public:
        constexpr SEPExecutionVariant(
            ExecutionMode execution_mode,
            MessagePassing message_passing,
            SchedulingMode scheduling_mode) noexcept
            : execution_mode_(execution_mode),
              message_passing_(message_passing),
              scheduling_mode_(scheduling_mode) {}

        constexpr ExecutionMode execution_mode() const noexcept
        {
            return execution_mode_;
        }

        constexpr MessagePassing message_passing() const noexcept
        {
            return message_passing_;
        }

        constexpr SchedulingMode scheduling_mode() const noexcept
        {
            return scheduling_mode_;
        }

        constexpr bool operator==(const SEPExecutionVariant &other) const noexcept
        {
            return execution_mode_ == other.execution_mode_ &&
                   message_passing_ == other.message_passing_ &&
                   scheduling_mode_ == other.scheduling_mode_;
        }

        constexpr bool operator!=(const SEPExecutionVariant &other) const noexcept
        {
            return !(*this == other);
        }

        /**
         * Deterministic ordering.
         *
         * Ordering is lexicographic over the three SEP dimensions. This makes
         * variants safe to use in ordered containers and gives tests a stable
         * representation without depending on pointer identity.
         */
        constexpr bool operator<(const SEPExecutionVariant &other) const noexcept
        {
            if (execution_mode_ != other.execution_mode_)
            {
                return static_cast<int>(execution_mode_) <
                       static_cast<int>(other.execution_mode_);
            }

            if (message_passing_ != other.message_passing_)
            {
                return static_cast<int>(message_passing_) <
                       static_cast<int>(other.message_passing_);
            }

            return static_cast<int>(scheduling_mode_) <
                   static_cast<int>(other.scheduling_mode_);
        }

        /**
         * Stable human-readable name matching SEP terminology.
         *
         * Examples:
         *   SYNC_PUSH_TD
         *   ASYNC_PUSH_DD
         *   SYNC_PULL_TD
         *   ASYNC_PULL_DD
         */
        std::string to_string() const
        {
            std::string result;
            result.reserve(18);

            result += execution_mode_ == ExecutionMode::SYNC
                          ? "SYNC_"
                          : "ASYNC_";

            result += message_passing_ == MessagePassing::PUSH
                          ? "PUSH_"
                          : "PULL_";

            result += scheduling_mode_ == SchedulingMode::DATA_DRIVEN
                          ? "DD"
                          : "TD";

            return result;
        }

        /**
         * Returns all eight combinations in deterministic order.
         *
         * The returned order is part of this project's representation contract:
         * it is derived from the enum values and does not depend on container
         * iteration order or registration order.
         */
        static constexpr std::array<SEPExecutionVariant, 8> all() noexcept
        {
            return {{
                {ExecutionMode::SYNC,
                 MessagePassing::PUSH,
                 SchedulingMode::DATA_DRIVEN},

                {ExecutionMode::SYNC,
                 MessagePassing::PUSH,
                 SchedulingMode::TOPOLOGY_DRIVEN},

                {ExecutionMode::SYNC,
                 MessagePassing::PULL,
                 SchedulingMode::DATA_DRIVEN},

                {ExecutionMode::SYNC,
                 MessagePassing::PULL,
                 SchedulingMode::TOPOLOGY_DRIVEN},

                {ExecutionMode::ASYNC,
                 MessagePassing::PUSH,
                 SchedulingMode::DATA_DRIVEN},

                {ExecutionMode::ASYNC,
                 MessagePassing::PUSH,
                 SchedulingMode::TOPOLOGY_DRIVEN},

                {ExecutionMode::ASYNC,
                 MessagePassing::PULL,
                 SchedulingMode::DATA_DRIVEN},

                {ExecutionMode::ASYNC,
                 MessagePassing::PULL,
                 SchedulingMode::TOPOLOGY_DRIVEN},
            }};
        }

        /**
         * Construct a variant from its stable string representation.
         *
         * Accepted spellings are exactly the names emitted by to_string().
         * Invalid values throw std::invalid_argument rather than silently
         * selecting a different execution policy.
         */
        static SEPExecutionVariant from_string(std::string_view value)
        {
            if (value == "SYNC_PUSH_DD")
            {
                return {ExecutionMode::SYNC,
                        MessagePassing::PUSH,
                        SchedulingMode::DATA_DRIVEN};
            }

            if (value == "SYNC_PUSH_TD")
            {
                return {ExecutionMode::SYNC,
                        MessagePassing::PUSH,
                        SchedulingMode::TOPOLOGY_DRIVEN};
            }

            if (value == "SYNC_PULL_DD")
            {
                return {ExecutionMode::SYNC,
                        MessagePassing::PULL,
                        SchedulingMode::DATA_DRIVEN};
            }

            if (value == "SYNC_PULL_TD")
            {
                return {ExecutionMode::SYNC,
                        MessagePassing::PULL,
                        SchedulingMode::TOPOLOGY_DRIVEN};
            }

            if (value == "ASYNC_PUSH_DD")
            {
                return {ExecutionMode::ASYNC,
                        MessagePassing::PUSH,
                        SchedulingMode::DATA_DRIVEN};
            }

            if (value == "ASYNC_PUSH_TD")
            {
                return {ExecutionMode::ASYNC,
                        MessagePassing::PUSH,
                        SchedulingMode::TOPOLOGY_DRIVEN};
            }

            if (value == "ASYNC_PULL_DD")
            {
                return {ExecutionMode::ASYNC,
                        MessagePassing::PULL,
                        SchedulingMode::DATA_DRIVEN};
            }

            if (value == "ASYNC_PULL_TD")
            {
                return {ExecutionMode::ASYNC,
                        MessagePassing::PULL,
                        SchedulingMode::TOPOLOGY_DRIVEN};
            }

            throw std::invalid_argument(
                "Invalid SEP execution variant: " + std::string(value));
        }

        /**
         * Convenience predicates used by later execution-driver code.
         */
        constexpr bool is_synchronous() const noexcept
        {
            return execution_mode_ == ExecutionMode::SYNC;
        }

        constexpr bool is_asynchronous() const noexcept
        {
            return execution_mode_ == ExecutionMode::ASYNC;
        }

        constexpr bool is_push() const noexcept
        {
            return message_passing_ == MessagePassing::PUSH;
        }

        constexpr bool is_pull() const noexcept
        {
            return message_passing_ == MessagePassing::PULL;
        }

        constexpr bool is_data_driven() const noexcept
        {
            return scheduling_mode_ == SchedulingMode::DATA_DRIVEN;
        }

        constexpr bool is_topology_driven() const noexcept
        {
            return scheduling_mode_ == SchedulingMode::TOPOLOGY_DRIVEN;
        }

    private:
        ExecutionMode execution_mode_;
        MessagePassing message_passing_;
        SchedulingMode scheduling_mode_;
    };

} // namespace hytgraph::sep

#endif // HYTGRAPH_SEP_EXECUTION_VARIANT_HPP