#include "scheduling/contribution_scheduler.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace hytgraph::scheduling
{

    ContributionScheduler::ContributionScheduler(
        ContributionSchedulerOptions options)
        : options_(options)
    {
    }

    const ContributionSchedulerOptions &
    ContributionScheduler::options() const noexcept
    {
        return options_;
    }

    ContributionSchedulingPlan ContributionScheduler::schedule(
        const std::vector<ContributionPriority> &priorities) const
    {
        ContributionSchedulingPlan result;

        result.metrics.input_item_count = priorities.size();
        result.metrics.scheduled_item_count = priorities.size();

        std::vector<std::size_t> item_indices;
        item_indices.reserve(priorities.size());

        for (const auto &priority : priorities)
        {
            item_indices.push_back(priority.item_index);
        }

        std::sort(item_indices.begin(), item_indices.end());

        for (std::size_t position = 1U;
             position < item_indices.size();
             ++position)
        {
            if (item_indices[position] == item_indices[position - 1U])
            {
                ++result.metrics.redundant_work_count;
            }
        }

        result.ordered_item_indices.reserve(priorities.size());

        for (const auto &priority : priorities)
        {
            if (!std::isfinite(priority.contribution))
            {
                throw std::invalid_argument(
                    "contribution priority must be finite");
            }
        }

        std::vector<std::size_t> order(priorities.size());
        std::iota(order.begin(), order.end(), 0U);

        for (const auto &priority : priorities)
        {
            if (std::abs(priority.contribution) <=
                options_.zero_contribution_epsilon)
            {
                ++result.metrics.zero_contribution_count;
            }
        }

        if (options_.deterministic_tie_break)
        {
            std::stable_sort(
                order.begin(),
                order.end(),
                [&priorities](std::size_t lhs, std::size_t rhs)
                {
                    if (priorities[lhs].contribution !=
                        priorities[rhs].contribution)
                    {
                        return priorities[lhs].contribution >
                               priorities[rhs].contribution;
                    }

                    return priorities[lhs].item_index <
                           priorities[rhs].item_index;
                });
        }
        else
        {
            std::stable_sort(
                order.begin(),
                order.end(),
                [&priorities](std::size_t lhs, std::size_t rhs)
                {
                    return priorities[lhs].contribution >
                           priorities[rhs].contribution;
                });
        }

        for (std::size_t position = 0U; position < order.size(); ++position)
        {
            const auto original_position = order[position];

            result.ordered_item_indices.push_back(
                priorities[original_position].item_index);

            if (original_position != position)
            {
                ++result.metrics.reordered_item_count;
            }
        }

        result.metrics.scheduling_overhead =
            estimate_scheduling_overhead(priorities);

        return result;
    }

    ContributionSchedulingPlan
    ContributionScheduler::synchronous_reference(
        const std::vector<ContributionPriority> &priorities)
    {
        ContributionSchedulingPlan result;

        result.metrics.input_item_count = priorities.size();
        result.metrics.scheduled_item_count = priorities.size();

        result.ordered_item_indices.reserve(priorities.size());

        for (const auto &priority : priorities)
        {
            result.ordered_item_indices.push_back(priority.item_index);

            if (std::abs(priority.contribution) == 0.0)
            {
                ++result.metrics.zero_contribution_count;
            }
        }

        return result;
    }

    ContributionSchedulingMetrics
    ContributionScheduler::measure_work_metrics(
        const std::vector<ContributionPriority> &priorities)
    {
        ContributionSchedulingMetrics metrics;

        metrics.input_item_count = priorities.size();
        metrics.scheduled_item_count = priorities.size();

        std::vector<std::size_t> item_indices;
        item_indices.reserve(priorities.size());

        for (const auto &priority : priorities)
        {
            item_indices.push_back(priority.item_index);

            if (std::abs(priority.contribution) == 0.0)
            {
                ++metrics.zero_contribution_count;
            }
        }

        std::sort(item_indices.begin(), item_indices.end());

        for (std::size_t position = 1U;
             position < item_indices.size();
             ++position)
        {
            if (item_indices[position] == item_indices[position - 1U])
            {
                ++metrics.redundant_work_count;
            }
        }

        return metrics;
    }

    ContributionSchedulingMetrics
    ContributionScheduler::measure_work_metrics(
        const std::vector<ContributionPriority> &priorities,
        const std::vector<ContributionWorkObservation> &observations)
    {
        ContributionSchedulingMetrics metrics =
            measure_work_metrics(priorities);

        for (const auto &observation : observations)
        {
            if (observation.stale)
            {
                ++metrics.stale_work_count;
            }
        }

        return metrics;
    }

    std::vector<ContributionPriority>
    make_contribution_priorities(
        const std::vector<std::size_t> &item_indices,
        const std::vector<double> &contributions)
    {
        if (item_indices.size() != contributions.size())
        {
            throw std::invalid_argument(
                "contribution priority inputs must have equal size");
        }

        std::vector<ContributionPriority> priorities;
        priorities.reserve(item_indices.size());

        for (std::size_t index = 0U;
             index < item_indices.size();
             ++index)
        {
            priorities.push_back(ContributionPriority{
                item_indices[index],
                contributions[index]});
        }

        return priorities;
    }

    double ContributionScheduler::estimate_scheduling_overhead(
        const std::vector<ContributionPriority> &priorities)
    {
        if (priorities.empty())
        {
            return 0.0;
        }

        // Reference CPU model: estimate the ordering work as
        // N * log2(N), normalized by the number of input items.
        //
        // This is a planning-layer estimate, not a measured GPU/runtime
        // execution time.
        const double item_count =
            static_cast<double>(priorities.size());

        return std::log2(item_count);
    }

} // namespace hytgraph::scheduling