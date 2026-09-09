#pragma once

#include "transfer/hytm_cost_model.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace hytgraph::scheduling
{
    // Reference logical description of one executable task produced from
    // fine-grained HyTGraph logical partitions.
    //
    // The logical partitions remain the unit of HyTM cost analysis.  A task
    // is only an execution-level grouping of those already-selected
    // partitions.
    struct ExecutableTask
    {
        // Sequential ID in the combined task plan.
        std::size_t task_index = 0U;

        // Transfer engine shared by every logical partition represented by
        // this task.
        transfer::TransferEngine engine =
            transfer::TransferEngine::ExpTMFilter;

        // Indices of the logical partitions represented by this task.
        //
        // These indices refer to the input HyTM decision vector and are
        // retained so correctness can be checked against the original
        // per-partition decisions.
        std::vector<std::size_t> partition_indices;

        // Inclusive logical-partition index of the first partition in the
        // task.  For non-filter tasks this is still useful as a stable
        // ordering marker.
        std::size_t first_partition_index = 0U;

        // Exclusive logical-partition index immediately after the task's
        // final partition when the grouped partitions form a contiguous
        // range.  For non-contiguous future extensions this field is still
        // retained as metadata but the partition_indices vector remains
        // authoritative.
        std::size_t end_partition_index = 0U;

        [[nodiscard]] std::size_t partition_count() const noexcept
        {
            return partition_indices.size();
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return partition_indices.empty();
        }
    };

    // Summary metrics for the transformation from logical partitions to
    // executable tasks.
    struct TaskCombinationMetrics
    {
        std::size_t logical_partition_count = 0U;
        std::size_t executable_task_count = 0U;

        // Number of logical partitions absorbed by filter grouping beyond
        // the first partition in each filter task.
        std::size_t filter_partitions_combined = 0U;

        // Number of logical partitions represented by compaction tasks.
        std::size_t compaction_partitions_combined = 0U;

        // Number of logical partitions represented by zero-copy tasks.
        std::size_t zero_copy_partitions_combined = 0U;

        [[nodiscard]] bool task_count_reduced() const noexcept
        {
            return executable_task_count < logical_partition_count;
        }

        [[nodiscard]] std::size_t
        task_count_reduction() const noexcept
        {
            return logical_partition_count >= executable_task_count
                       ? logical_partition_count - executable_task_count
                       : 0U;
        }
    };

    // Result of the reference Phase 9 task-combining pass.
    struct TaskCombinationPlan
    {
        // One executable task descriptor for each grouped task.
        std::vector<ExecutableTask> tasks;

        TaskCombinationMetrics metrics;

        [[nodiscard]] std::size_t executable_task_count() const noexcept
        {
            return tasks.size();
        }
    };

    struct TaskCombinerOptions
    {
        // HyTGraph paper value for consecutive ExpTM-Filter task grouping.
        //
        // The configuration remains explicit so experiments can vary it
        // independently without changing the logical partitioning.
        std::size_t filter_combine_k = 4U;
    };

    // CPU/reference task-combination layer for Phase 9.
    //
    // Responsibilities:
    //
    //   1. preserve the per-partition HyTM engine decisions;
    //   2. group consecutive ExpTM-Filter partitions in chunks of at most k;
    //   3. group all selected ExpTM-Compaction partitions into task-level
    //      units;
    //   4. group all selected ImpTM-Zero-Copy partitions into task-level
    //      units;
    //   5. expose logical-partition/task-count metrics.
    //
    // This layer does not:
    //
    //   - recompute HyTM costs;
    //   - modify logical partition boundaries;
    //   - allocate GPU memory;
    //   - perform cudaMemcpy;
    //   - execute CUDA kernels;
    //   - schedule CUDA streams;
    //   - perform contribution-driven scheduling.
    class TaskCombiner
    {
    public:
        explicit TaskCombiner(
            TaskCombinerOptions options = {});

        [[nodiscard]] const TaskCombinerOptions &
        options() const noexcept;

        // Combine an ordered sequence of HyTM per-partition engine
        // decisions.
        //
        // The input order is authoritative and is preserved.
        //
        // Phase 9 grouping rules:
        //
        //   ExpTM-Filter:
        //       group consecutive filter partitions in batches of up to k.
        //
        //   ExpTM-Compaction:
        //       combine the selected compaction partitions into one logical
        //       executable task.
        //
        //   ImpTM-Zero-Copy:
        //       combine the selected zero-copy partitions into one logical
        //       executable task.
        //
        // The implementation must not change any individual partition's
        // engine selection.
        [[nodiscard]] TaskCombinationPlan combine(
            const std::vector<transfer::TransferEngine> &
                partition_engines) const;

    private:
        TaskCombinerOptions options_;
    };

} // namespace hytgraph::scheduling