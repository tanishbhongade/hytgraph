#ifndef HYTGRAPH_SEP_EXECUTION_CONFIG_HPP
#define HYTGRAPH_SEP_EXECUTION_CONFIG_HPP

#include "sep_execution_variant.hpp"

namespace hytgraph::sep
{

    /**
     * Phase 12 execution configuration.
     *
     * This is the project-local equivalent of the execution-policy portion of
     * SEP-Graph's EngineOptions / AlgoVariant configuration.
     *
     * It deliberately does NOT contain:
     *   - graph data;
     *   - graph partitions;
     *   - activity/worklist state;
     *   - task state;
     *   - CUDA streams;
     *   - transfer strategy;
     *   - Filter / Compaction / Zero-Copy;
     *   - VCGC;
     *   - load-balancing implementation.
     *
     * Those concerns remain outside the SEP execution foundation.
     */
    class SEPExecutionConfig
    {
    public:
        /**
         * Construct a configuration using the project's neutral default.
         *
         * SYNC_PUSH_DD is used as the default because it provides a deterministic
         * execution-policy representation without requiring any runtime policy
         * selection machinery in Phase 12.
         *
         * No execution is performed by this class.
         */
        constexpr SEPExecutionConfig() noexcept
            : variant_(ExecutionMode::SYNC,
                       MessagePassing::PUSH,
                       SchedulingMode::DATA_DRIVEN) {}

        explicit constexpr SEPExecutionConfig(
            SEPExecutionVariant variant) noexcept
            : variant_(variant) {}

        constexpr const SEPExecutionVariant &variant() const noexcept
        {
            return variant_;
        }

        constexpr ExecutionMode execution_mode() const noexcept
        {
            return variant_.execution_mode();
        }

        constexpr MessagePassing message_passing() const noexcept
        {
            return variant_.message_passing();
        }

        constexpr SchedulingMode scheduling_mode() const noexcept
        {
            return variant_.scheduling_mode();
        }

        constexpr bool is_synchronous() const noexcept
        {
            return variant_.is_synchronous();
        }

        constexpr bool is_asynchronous() const noexcept
        {
            return variant_.is_asynchronous();
        }

        constexpr bool is_push() const noexcept
        {
            return variant_.is_push();
        }

        constexpr bool is_pull() const noexcept
        {
            return variant_.is_pull();
        }

        constexpr bool is_data_driven() const noexcept
        {
            return variant_.is_data_driven();
        }

        constexpr bool is_topology_driven() const noexcept
        {
            return variant_.is_topology_driven();
        }

        std::string to_string() const
        {
            return variant_.to_string();
        }

        /**
         * Replace the selected execution variant.
         *
         * This changes policy only. It does not initialize or mutate any
         * execution state.
         */
        constexpr void set_variant(SEPExecutionVariant variant) noexcept
        {
            variant_ = variant;
        }

        /**
         * Explicit constructors for all SEP execution dimensions.
         *
         * These methods make later driver/application code readable without
         * exposing the representation of SEPExecutionVariant.
         */
        constexpr void set_execution_mode(ExecutionMode mode) noexcept
        {
            variant_ = SEPExecutionVariant(
                mode,
                variant_.message_passing(),
                variant_.scheduling_mode());
        }

        constexpr void set_message_passing(MessagePassing passing) noexcept
        {
            variant_ = SEPExecutionVariant(
                variant_.execution_mode(),
                passing,
                variant_.scheduling_mode());
        }

        constexpr void set_scheduling_mode(SchedulingMode scheduling) noexcept
        {
            variant_ = SEPExecutionVariant(
                variant_.execution_mode(),
                variant_.message_passing(),
                scheduling);
        }

        /**
         * Construct directly from SEP's canonical string representation.
         *
         * Example:
         *   auto config =
         *       SEPExecutionConfig::from_string("ASYNC_PULL_DD");
         */
        static SEPExecutionConfig from_string(std::string_view value)
        {
            return SEPExecutionConfig(SEPExecutionVariant::from_string(value));
        }

    private:
        SEPExecutionVariant variant_;
    };

} // namespace hytgraph::sep

#endif // HYTGRAPH_SEP_EXECUTION_CONFIG_HPP