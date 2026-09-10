#pragma once

#include <cstddef>
#include <vector>

namespace hytgraph::scheduling
{

    // Reference scheduling policy used to order already-identified
    // logical work items by their contribution/delta.
    //
    // The scheduler does not compute algorithm-specific contributions.
    // PageRank and SSSP execution layers provide the contribution values.
    //
    // This keeps Phase 11 independent from the later SEP-Graph execution
    // layer and preserves the synchronous execution path as a reference.
    enum class ContributionSchedulingPolicy
    {
        PageRankDelta,
        SSSPContribution
    };

    // One logical work item's contribution priority.
    struct ContributionPriority
    {
        // Index of the logical partition or work item.
        std::size_t item_index = 0U;

        // Aggregate contribution/delta associated with this item.
        //
        // Larger values receive higher priority.
        double contribution = 0.0;
    };

    // Metrics for the reference contribution-driven scheduling pass.
    struct ContributionSchedulingMetrics
    {
        // Number of logical work items presented to the scheduler.
        std::size_t input_item_count = 0U;

        // Number of items returned by the scheduler.
        std::size_t scheduled_item_count = 0U;

        // Number of items whose position differs from the synchronous
        // reference ordering.
        std::size_t reordered_item_count = 0U;

        // Number of items with zero contribution.
        //
        // These are useful for identifying work that is unlikely to make
        // immediate progress, but this metric does not by itself claim that
        // such work is stale.
        std::size_t zero_contribution_count = 0U;

        // Scheduling overhead measured by the caller/runtime.
        //
        // The scheduler itself does not fabricate a hardware timing value.
        double scheduling_overhead = 0.0;

        std::size_t stale_work_count = 0U;
        std::size_t redundant_work_count = 0U;
    };

    // Result of the CPU/reference contribution-driven scheduling pass.
    struct ContributionSchedulingPlan
    {
        // Item indices in execution priority order.
        //
        // The first element has the highest contribution priority.
        std::vector<std::size_t> ordered_item_indices;

        ContributionSchedulingMetrics metrics;

        [[nodiscard]] std::size_t size() const noexcept
        {
            return ordered_item_indices.size();
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return ordered_item_indices.empty();
        }

        [[nodiscard]] bool reordered() const noexcept
        {
            return metrics.reordered_item_count != 0U;
        }
    };

    struct ContributionSchedulerOptions
    {
        // Scheduling policy used to interpret the supplied contribution
        // values.
        ContributionSchedulingPolicy policy =
            ContributionSchedulingPolicy::PageRankDelta;

        // When true, equal contribution values are resolved by ascending
        // item index. This provides deterministic reference behavior.
        bool deterministic_tie_break = true;

        // Contributions whose absolute value is at or below this threshold
        // are treated as zero for the zero-contribution metric.
        //
        // This is a numerical/reference threshold, not a paper-defined
        // hardware parameter.
        double zero_contribution_epsilon = 0.0;
    };

    struct ContributionWorkObservation
    {
        std::size_t item_index = 0U;

        // True when the work item was processed using contribution state
        // that was no longer current when the item executed.
        bool stale = false;
    };

    // CPU/reference contribution-driven scheduler for Phase 11.
    //
    // Responsibilities:
    //
    //   1. accept contribution/delta values produced by an algorithm;
    //   2. prioritize larger contributions;
    //   3. preserve a deterministic synchronous reference ordering;
    //   4. expose scheduling/reordering metrics.
    //
    // This class does not:
    //
    //   - compute PageRank deltas;
    //   - compute SSSP relaxations;
    //   - execute graph kernels;
    //   - manage CUDA streams;
    //   - manage SEP-Graph worklists;
    //   - perform asynchronous GPU execution;
    //   - modify logical partition boundaries;
    //   - perform HyTM engine selection;
    //   - perform task combining.
    //
    // The paper describes contribution/delta-driven prioritization, but does
    // not specify enough implementation detail to reproduce the original
    // scheduler's internal queue/worklist implementation. This class is
    // therefore the project's CPU/reference scheduling abstraction.

    [[nodiscard]] std::vector<ContributionPriority>
    make_contribution_priorities(
        const std::vector<std::size_t> &item_indices,
        const std::vector<double> &contributions);

    class ContributionScheduler
    {
    public:
        explicit ContributionScheduler(
            ContributionSchedulerOptions options = {});

        [[nodiscard]] const ContributionSchedulerOptions &
        options() const noexcept;

        // Order work items by descending contribution.
        //
        // The input item_index values are preserved and are returned in
        // scheduling order. The contribution values themselves are not
        // modified.
        //
        // For equal contributions, item_index is used as a deterministic
        // tie breaker when deterministic_tie_break is enabled.
        [[nodiscard]] ContributionSchedulingPlan schedule(
            const std::vector<ContributionPriority> &priorities) const;

        // Produce the synchronous reference ordering.
        //
        // This ordering is independent of contribution values and preserves
        // the supplied input order.
        [[nodiscard]] static ContributionSchedulingPlan
        synchronous_reference(
            const std::vector<ContributionPriority> &priorities);

        [[nodiscard]] static ContributionSchedulingMetrics
        measure_work_metrics(
            const std::vector<ContributionPriority> &priorities);

        [[nodiscard]] static ContributionSchedulingMetrics
        measure_work_metrics(
            const std::vector<ContributionPriority> &priorities,
            const std::vector<ContributionWorkObservation> &observations);

        [[nodiscard]] std::vector<ContributionPriority>
        make_contribution_priorities(
            const std::vector<std::size_t> &item_indices,
            const std::vector<double> &contributions);

        [[nodiscard]] static double
        estimate_scheduling_overhead(
            const std::vector<ContributionPriority> &priorities);

    private:
        ContributionSchedulerOptions options_;
    };

} // namespace hytgraph::scheduling