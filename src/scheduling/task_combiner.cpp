#include "scheduling/task_combiner.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace hytgraph::scheduling
{
    namespace
    {
        using transfer::TransferEngine;

        void validate_options(const TaskCombinerOptions &options)
        {
            if (options.filter_combine_k == 0U)
            {
                throw std::invalid_argument(
                    "TaskCombiner: filter_combine_k must be greater than zero");
            }
        }

        ExecutableTask make_task(
            TransferEngine engine,
            const std::vector<std::size_t> &partition_indices)
        {
            ExecutableTask task;

            task.engine = engine;
            task.partition_indices = partition_indices;

            if (!partition_indices.empty())
            {
                task.first_partition_index =
                    partition_indices.front();

                /*
                 * partition_indices is the authoritative membership list.
                 *
                 * For consecutive groups, this is also the exclusive
                 * partition-range endpoint. For globally combined
                 * compaction/zero-copy tasks, intervening partitions may belong
                 * to another engine, so callers must not interpret this field
                 * as the complete membership range.
                 */
                task.end_partition_index =
                    partition_indices.back() + 1U;
            }

            return task;
        }

    } // namespace

    TaskCombiner::TaskCombiner(
        TaskCombinerOptions options)
        : options_(std::move(options))
    {
        validate_options(options_);
    }

    const TaskCombinerOptions &
    TaskCombiner::options() const noexcept
    {
        return options_;
    }

    TaskCombinationPlan TaskCombiner::combine(
        const std::vector<transfer::TransferEngine> &
            partition_engines) const
    {
        validate_options(options_);

        TaskCombinationPlan result;

        result.metrics.logical_partition_count =
            partition_engines.size();

        if (partition_engines.empty())
        {
            return result;
        }

        /*
         * ExpTM-Filter is grouped only with consecutive filter partitions.
         *
         * ExpTM-Compaction and ImpTM-Zero-Copy are accumulated separately so
         * that all partitions selected for the same engine can subsequently
         * form one executable task.
         */
        std::vector<std::size_t> compaction_partitions;
        std::vector<std::size_t> zero_copy_partitions;

        std::vector<ExecutableTask> tasks;

        std::size_t index = 0U;

        while (index < partition_engines.size())
        {
            const TransferEngine engine =
                partition_engines[index];

            if (engine == TransferEngine::ExpTMFilter)
            {
                /*
                 * Paper-aligned filter grouping:
                 *
                 *   - partitions must be consecutive;
                 *   - no task contains more than k partitions.
                 */
                std::vector<std::size_t> filter_group;

                filter_group.reserve(
                    std::min(
                        options_.filter_combine_k,
                        partition_engines.size() - index));

                while (
                    index < partition_engines.size() &&
                    partition_engines[index] ==
                        TransferEngine::ExpTMFilter &&
                    filter_group.size() <
                        options_.filter_combine_k)
                {
                    filter_group.push_back(index);
                    ++index;
                }

                tasks.push_back(
                    make_task(
                        TransferEngine::ExpTMFilter,
                        filter_group));

                continue;
            }

            if (engine == TransferEngine::ExpTMCompaction)
            {
                compaction_partitions.push_back(index);
                ++index;
                continue;
            }

            if (engine == TransferEngine::ImpTMZeroCopy)
            {
                zero_copy_partitions.push_back(index);
                ++index;
                continue;
            }

            /*
             * TransferEngine currently has exactly the three engines above.
             * Reaching this branch means the enum and Phase 9 implementation
             * have diverged.
             */
            throw std::invalid_argument(
                "TaskCombiner: unsupported transfer engine");
        }

        /*
         * Phase 9 combines all partitions selected for ExpTM-Compaction into
         * one logical executable task.
         */
        if (!compaction_partitions.empty())
        {
            tasks.push_back(
                make_task(
                    TransferEngine::ExpTMCompaction,
                    compaction_partitions));
        }

        /*
         * Phase 9 combines all partitions selected for ImpTM-Zero-Copy into
         * one logical executable task.
         */
        if (!zero_copy_partitions.empty())
        {
            tasks.push_back(
                make_task(
                    TransferEngine::ImpTMZeroCopy,
                    zero_copy_partitions));
        }

        /*
         * Compaction and zero-copy partitions were accumulated separately.
         * Restore deterministic ordering based on the first logical
         * partition represented by each executable task.
         */
        std::sort(
            tasks.begin(),
            tasks.end(),
            [](const ExecutableTask &lhs,
               const ExecutableTask &rhs)
            {
                return lhs.first_partition_index <
                       rhs.first_partition_index;
            });

        for (std::size_t task_index = 0U;
             task_index < tasks.size();
             ++task_index)
        {
            tasks[task_index].task_index = task_index;
        }

        result.tasks = std::move(tasks);

        result.metrics.executable_task_count =
            result.tasks.size();

        result.metrics.executable_task_count =
            result.tasks.size();

        /*
         * Calculate combination metrics from the final executable-task plan.
         */
        for (const ExecutableTask &task : result.tasks)
        {
            const std::size_t count =
                task.partition_count();

            if (task.engine == TransferEngine::ExpTMFilter)
            {
                /*
                 * A filter task containing N partitions represents N-1
                 * combinations beyond its first partition.
                 */
                if (count > 1U)
                {
                    result.metrics.filter_partitions_combined +=
                        count - 1U;
                }
            }
            else if (task.engine ==
                     TransferEngine::ExpTMCompaction)
            {
                result.metrics.compaction_partitions_combined +=
                    count;
            }
            else if (task.engine ==
                     TransferEngine::ImpTMZeroCopy)
            {
                result.metrics.zero_copy_partitions_combined +=
                    count;
            }
        }

        return result;
    }

} // namespace hytgraph::scheduling