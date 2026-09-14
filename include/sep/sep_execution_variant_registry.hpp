#ifndef HYTGRAPH_SEP_EXECUTION_VARIANT_REGISTRY_HPP
#define HYTGRAPH_SEP_EXECUTION_VARIANT_REGISTRY_HPP

#include <array>
#include <cstddef>
#include <string_view>

#include "sep_execution_variant.hpp"

namespace hytgraph::sep
{

    /**
     * Canonical registry of the eight SEP execution combinations.
     *
     * The execution dimensions are:
     *
     *     ExecutionMode
     *     MessagePassing
     *     SchedulingMode
     *
     * producing:
     *
     *     SYNC_PUSH_DD
     *     SYNC_PUSH_TD
     *     SYNC_PULL_DD
     *     SYNC_PULL_TD
     *     ASYNC_PUSH_DD
     *     ASYNC_PUSH_TD
     *     ASYNC_PULL_DD
     *     ASYNC_PULL_TD
     *
     * This registry performs no runtime execution and owns no graph state.
     *
     * The ordering is intentionally stable so configuration parsing,
     * diagnostics, and unit tests have deterministic behavior.
     */
    class SEPExecutionVariantRegistry
    {
    public:
        static constexpr std::size_t kVariantCount = 8;

        using variant_array =
            std::array<SEPExecutionVariant, kVariantCount>;

        /**
         * Return the canonical registry.
         *
         * The array has static storage duration, so returning a reference
         * here is safe.
         */
        static constexpr const variant_array &all() noexcept
        {
            return variants_;
        }

        /**
         * Number of registered SEP execution variants.
         */
        static constexpr std::size_t size() noexcept
        {
            return kVariantCount;
        }

        /**
         * Access a variant by its stable registry index.
         *
         * Caller must provide an index in [0, size()).
         */
        static constexpr const SEPExecutionVariant &at(
            std::size_t index) noexcept
        {
            return variants_[index];
        }

        /**
         * Find a variant by canonical name.
         *
         * Returns nullptr if the name is not registered.
         */
        static const SEPExecutionVariant *find(
            std::string_view name) noexcept
        {
            for (const auto &variant : variants_)
            {
                if (variant.to_string() == name)
                {
                    return &variant;
                }
            }

            return nullptr;
        }

        /**
         * Return whether a canonical variant name is registered.
         */
        static bool contains(
            std::string_view name) noexcept
        {
            return find(name) != nullptr;
        }

    private:
        /**
         * Static constexpr storage is required here.
         *
         * DO NOT replace this with a function returning an array by value
         * and then return a reference to that result.
         */
        inline static constexpr variant_array variants_ = {{SEPExecutionVariant(
                                                                ExecutionMode::SYNC,
                                                                MessagePassing::PUSH,
                                                                SchedulingMode::DATA_DRIVEN),

                                                            SEPExecutionVariant(
                                                                ExecutionMode::SYNC,
                                                                MessagePassing::PUSH,
                                                                SchedulingMode::TOPOLOGY_DRIVEN),

                                                            SEPExecutionVariant(
                                                                ExecutionMode::SYNC,
                                                                MessagePassing::PULL,
                                                                SchedulingMode::DATA_DRIVEN),

                                                            SEPExecutionVariant(
                                                                ExecutionMode::SYNC,
                                                                MessagePassing::PULL,
                                                                SchedulingMode::TOPOLOGY_DRIVEN),

                                                            SEPExecutionVariant(
                                                                ExecutionMode::ASYNC,
                                                                MessagePassing::PUSH,
                                                                SchedulingMode::DATA_DRIVEN),

                                                            SEPExecutionVariant(
                                                                ExecutionMode::ASYNC,
                                                                MessagePassing::PUSH,
                                                                SchedulingMode::TOPOLOGY_DRIVEN),

                                                            SEPExecutionVariant(
                                                                ExecutionMode::ASYNC,
                                                                MessagePassing::PULL,
                                                                SchedulingMode::DATA_DRIVEN),

                                                            SEPExecutionVariant(
                                                                ExecutionMode::ASYNC,
                                                                MessagePassing::PULL,
                                                                SchedulingMode::TOPOLOGY_DRIVEN)}};
    };

} // namespace hytgraph::sep

#endif // HYTGRAPH_SEP_EXECUTION_VARIANT_REGISTRY_HPP