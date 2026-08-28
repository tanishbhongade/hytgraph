#include "transfer/filter_engine.hpp"

#include <stdexcept>

namespace hytgraph::transfer
{
    namespace
    {

        void validate_partitions(
            const graph::CSRGraph &graph,
            const std::vector<graph::LogicalPartition> &partitions)
        {
            const auto vertex_count = graph.num_vertices();
            const auto edge_count = graph.num_edges();

            graph::CSRGraph::vertex_id expected_vertex_begin = 0;
            graph::CSRGraph::offset_type expected_edge_begin = 0;

            for (const graph::LogicalPartition &partition : partitions)
            {
                if (partition.vertex_begin() != expected_vertex_begin)
                {
                    throw std::invalid_argument(
                        "logical partitions must cover vertices contiguously");
                }

                if (partition.edge_begin() != expected_edge_begin)
                {
                    throw std::invalid_argument(
                        "logical partitions must cover edges contiguously");
                }

                if (partition.vertex_begin() > partition.vertex_end())
                {
                    throw std::invalid_argument(
                        "logical partition has invalid vertex range");
                }

                if (partition.edge_begin() > partition.edge_end())
                {
                    throw std::invalid_argument(
                        "logical partition has invalid edge range");
                }

                if (static_cast<graph::CSRGraph::offset_type>(
                        partition.vertex_end()) > vertex_count)
                {
                    throw std::out_of_range(
                        "logical partition vertex range exceeds graph");
                }

                if (partition.edge_end() > edge_count)
                {
                    throw std::out_of_range(
                        "logical partition edge range exceeds graph");
                }

                expected_vertex_begin = partition.vertex_end();
                expected_edge_begin = partition.edge_end();
            }

            if (expected_vertex_begin != vertex_count)
            {
                throw std::invalid_argument(
                    "logical partitions do not cover all graph vertices");
            }

            if (expected_edge_begin != edge_count)
            {
                throw std::invalid_argument(
                    "logical partitions do not cover all graph edges");
            }
        }

    } // namespace

    std::size_t
    FilterTransferPlan::active_partition_count() const noexcept
    {
        std::size_t count = 0U;

        for (const FilterPartitionDecision &partition : partitions)
        {
            if (partition.active)
            {
                ++count;
            }
        }

        return count;
    }

    std::size_t
    FilterTransferPlan::transferred_partition_count() const noexcept
    {
        std::size_t count = 0U;

        for (const FilterPartitionDecision &partition : partitions)
        {
            if (partition.transferred)
            {
                ++count;
            }
        }

        return count;
    }

    graph::CSRGraph::offset_type
    FilterTransferPlan::transferred_edge_count() const noexcept
    {
        graph::CSRGraph::offset_type count = 0U;

        for (const FilterPartitionDecision &partition : partitions)
        {
            count += partition.transferred_edges;
        }

        return count;
    }

    std::size_t
    FilterTransferPlan::transferred_byte_count() const noexcept
    {
        std::size_t count = 0U;

        for (const FilterPartitionDecision &partition : partitions)
        {
            count += partition.transferred_bytes;
        }

        return count;
    }

    ExpTMFilter::ExpTMFilter(bool enabled) noexcept
        : enabled_(enabled)
    {
    }

    bool ExpTMFilter::enabled() const noexcept
    {
        return enabled_;
    }

    FilterTransferPlan ExpTMFilter::plan(
        const graph::CSRGraph &graph,
        const std::vector<graph::LogicalPartition> &partitions,
        const graph::ActivityTracker &activity) const
    {
        if (activity.vertex_count() != graph.num_vertices())
        {
            throw std::invalid_argument(
                "activity tracker vertex count does not match graph");
        }

        validate_partitions(graph, partitions);

        const std::vector<graph::ActivityTracker::PartitionActivity>
            statistics =
                activity.partition_statistics(
                    graph,
                    [&partitions]()
                    {
                        std::vector<graph::ActivityTracker::VertexRange>
                            ranges;

                        ranges.reserve(partitions.size());

                        for (const graph::LogicalPartition &partition :
                             partitions)
                        {
                            ranges.push_back(
                                graph::ActivityTracker::VertexRange{
                                    partition.vertex_begin(),
                                    partition.vertex_end()});
                        }

                        return ranges;
                    }());

        FilterTransferPlan result;
        result.partitions.reserve(partitions.size());

        for (std::size_t index = 0U; index < partitions.size(); ++index)
        {
            const graph::LogicalPartition &partition = partitions[index];
            const auto &activity_stats = statistics[index];

            FilterPartitionDecision decision;

            decision.partition_index = index;
            decision.active =
                activity_stats.active_edges != 0U;
            decision.active_edges =
                activity_stats.active_edges;
            decision.total_edges =
                partition.edge_count();

            const bool should_transfer =
                !enabled_ || decision.active;

            decision.transferred = should_transfer;

            if (should_transfer)
            {
                // ExpTM-Filter transfers the complete logical partition.
                // It does not compact active edges.
                decision.transferred_edges =
                    partition.edge_count();

                decision.transferred_bytes =
                    partition.edge_data_bytes();
            }

            result.partitions.push_back(decision);
        }

        return result;
    }

} // namespace hytgraph::transfer