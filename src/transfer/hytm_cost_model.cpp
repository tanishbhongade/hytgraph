#include "transfer/hytm_cost_model.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace hytgraph::transfer
{
    namespace
    {

        using VertexId = graph::CSRGraph::vertex_id;
        using OffsetType = graph::CSRGraph::offset_type;

        constexpr double kDefaultRtt = 1.0;

        void validate_options(const HyTMCostModelOptions &options)
        {
            if (!std::isfinite(options.alpha) ||
                !std::isfinite(options.beta) ||
                !std::isfinite(options.gamma))
            {
                throw std::invalid_argument(
                    "HyTMCostModel: alpha, beta, and gamma must be finite");
            }

            /*
             * The selector semantics require positive scaling factors.
             *
             * The paper specifies alpha = 0.80, beta = 0.40, and
             * gamma = 0.625. The reproduction keeps all three configurable,
             * while rejecting values that cannot represent a meaningful
             * cost-model configuration.
             */
            if (options.alpha <= 0.0)
            {
                throw std::invalid_argument(
                    "HyTMCostModel: alpha must be greater than zero");
            }

            if (options.beta <= 0.0)
            {
                throw std::invalid_argument(
                    "HyTMCostModel: beta must be greater than zero");
            }

            if (options.gamma < 0.0 || options.gamma > 1.0)
            {
                throw std::invalid_argument(
                    "HyTMCostModel: gamma must be in the range [0, 1]");
            }

            if (!std::isfinite(options.rtt) ||
                options.rtt < 0.0)
            {
                throw std::invalid_argument(
                    "HyTMCostModel: RTT must be finite and non-negative");
            }

            if (!std::isfinite(
                    options.cpu_compaction_throughput_bytes_per_second) ||
                options.cpu_compaction_throughput_bytes_per_second <= 0.0)
            {
                throw std::invalid_argument(
                    "HyTMCostModel: CPU compaction throughput must be "
                    "finite and greater than zero");
            }

            if (options.request_payload_bytes == 0U)
            {
                throw std::invalid_argument(
                    "HyTMCostModel: request payload size must be greater "
                    "than zero");
            }

            if (options.max_requests_per_tlp == 0U)
            {
                throw std::invalid_argument(
                    "HyTMCostModel: maximum requests per TLP must be "
                    "greater than zero");
            }

            if (options.destination_entry_bytes == 0U)
            {
                throw std::invalid_argument(
                    "HyTMCostModel: destination entry size must be "
                    "greater than zero");
            }

            if (options.vertex_index_bytes == 0U)
            {
                throw std::invalid_argument(
                    "HyTMCostModel: vertex index size must be greater "
                    "than zero");
            }

            if (options.alignment_bytes == 0U)
            {
                throw std::invalid_argument(
                    "HyTMCostModel: alignment size must be greater "
                    "than zero");
            }
        }

        void validate_activity(
            const graph::CSRGraph &graph,
            const graph::ActivityTracker &activity)
        {
            if (activity.vertex_count() != graph.num_vertices())
            {
                throw std::invalid_argument(
                    "HyTMCostModel: activity tracker vertex count does "
                    "not match graph");
            }
        }

        void validate_partition(
            const graph::CSRGraph &graph,
            const graph::LogicalPartition &partition)
        {
            const OffsetType vertex_count = graph.num_vertices();
            const OffsetType edge_count = graph.num_edges();

            if (static_cast<OffsetType>(partition.vertex_begin()) >
                static_cast<OffsetType>(partition.vertex_end()))
            {
                throw std::invalid_argument(
                    "HyTMCostModel: partition vertex range is invalid");
            }

            if (static_cast<OffsetType>(partition.vertex_end()) >
                vertex_count)
            {
                throw std::invalid_argument(
                    "HyTMCostModel: partition vertex range exceeds graph");
            }

            if (partition.edge_begin() > partition.edge_end())
            {
                throw std::invalid_argument(
                    "HyTMCostModel: partition edge range is invalid");
            }

            if (partition.edge_end() > edge_count)
            {
                throw std::invalid_argument(
                    "HyTMCostModel: partition edge range exceeds graph");
            }

            /*
             * Logical partitions are required to preserve complete CSR
             * rows. Therefore both partition boundaries must correspond to
             * row-offset boundaries.
             */
            const auto &row_offsets = graph.row_offsets();

            if (row_offsets[static_cast<std::size_t>(
                    partition.vertex_begin())] !=
                partition.edge_begin())
            {
                throw std::invalid_argument(
                    "HyTMCostModel: partition edge_begin does not match "
                    "its CSR row boundary");
            }

            if (row_offsets[static_cast<std::size_t>(
                    partition.vertex_end())] !=
                partition.edge_end())
            {
                throw std::invalid_argument(
                    "HyTMCostModel: partition edge_end does not match "
                    "its CSR row boundary");
            }
        }

        std::uint64_t checked_multiply(
            std::uint64_t lhs,
            std::uint64_t rhs,
            const char *message)
        {
            if (lhs != 0U &&
                rhs > std::numeric_limits<std::uint64_t>::max() / lhs)
            {
                throw std::overflow_error(message);
            }

            return lhs * rhs;
        }

        std::uint64_t checked_add(
            std::uint64_t lhs,
            std::uint64_t rhs,
            const char *message)
        {
            if (rhs >
                std::numeric_limits<std::uint64_t>::max() - lhs)
            {
                throw std::overflow_error(message);
            }

            return lhs + rhs;
        }

        std::uint64_t ceil_div(
            std::uint64_t numerator,
            std::uint64_t denominator)
        {
            if (denominator == 0U)
            {
                throw std::invalid_argument(
                    "HyTMCostModel: division denominator must be "
                    "greater than zero");
            }

            return numerator / denominator +
                   ((numerator % denominator) != 0U ? 1U : 0U);
        }

        /*
         * Convert a byte quantity into the number of saturated TLPs.
         *
         * The paper's expression is:
         *
         *   ceil(bytes / m / MR)
         *
         * For integer byte accounting this is equivalent to:
         *
         *   ceil(bytes / (m * MR))
         *
         * provided the product is represented without overflow.
         */
        std::uint64_t saturated_tlp_count(
            std::uint64_t bytes,
            const HyTMCostModelOptions &options)
        {
            const std::uint64_t payload =
                static_cast<std::uint64_t>(
                    options.request_payload_bytes);

            const std::uint64_t requests_per_tlp =
                static_cast<std::uint64_t>(
                    options.max_requests_per_tlp);

            const std::uint64_t bytes_per_tlp =
                checked_multiply(
                    payload,
                    requests_per_tlp,
                    "HyTMCostModel: TLP byte capacity overflow");

            return ceil_div(bytes, bytes_per_tlp);
        }

        /*
         * Compute the logical byte offset of a vertex's neighbor list.
         *
         * Phase 7 established this as the portable reference approximation
         * because CSRGraph does not expose the physical host-memory address
         * used by a real mapped allocation.
         */
        std::uint64_t logical_neighbor_byte_offset(
            const graph::CSRGraph &graph,
            VertexId vertex,
            const HyTMCostModelOptions &options)
        {
            const std::uint64_t edge_offset =
                graph.row_offsets()[static_cast<std::size_t>(vertex)];

            return checked_multiply(
                edge_offset,
                static_cast<std::uint64_t>(
                    options.destination_entry_bytes),
                "HyTMCostModel: logical neighbor byte offset overflow");
        }

        std::uint64_t zero_copy_vertex_requests(
            const graph::CSRGraph &graph,
            VertexId vertex,
            const HyTMCostModelOptions &options)
        {
            const std::uint64_t degree =
                graph.out_degree(vertex);

            if (degree == 0U)
            {
                return 0U;
            }

            const std::uint64_t bytes =
                checked_multiply(
                    degree,
                    static_cast<std::uint64_t>(
                        options.destination_entry_bytes),
                    "HyTMCostModel: zero-copy vertex byte count overflow");

            return ceil_div(
                bytes,
                static_cast<std::uint64_t>(
                    options.request_payload_bytes));
        }

        std::uint64_t zero_copy_alignment_overhead(
            const graph::CSRGraph &graph,
            VertexId vertex,
            const HyTMCostModelOptions &options)
        {
            /*
             * An empty adjacency list has no neighbor transfer and therefore
             * cannot require the additional transaction represented by am().
             */
            if (graph.out_degree(vertex) == 0U)
            {
                return 0U;
            }

            const std::uint64_t offset =
                logical_neighbor_byte_offset(
                    graph,
                    vertex,
                    options);

            return (offset %
                        static_cast<std::uint64_t>(
                            options.alignment_bytes) !=
                    0U)
                       ? 1U
                       : 0U;
        }

        HyTMPartitionMetrics collect_metrics(
            const graph::CSRGraph &graph,
            const graph::LogicalPartition &partition,
            const graph::ActivityTracker &activity,
            const HyTMCostModelOptions &options,
            std::size_t partition_index)
        {
            HyTMPartitionMetrics metrics;

            metrics.partition_index = partition_index;
            metrics.vertex_begin = partition.vertex_begin();
            metrics.vertex_end = partition.vertex_end();

            /*
             * ExpTM-Filter transfers the complete logical edge payload.
             *
             * The paper models this using the number of edges in Pi,
             * multiplied by d1.
             */
            metrics.total_edges =
                partition.edge_count();

            metrics.filter_transfer_bytes =
                checked_multiply(
                    static_cast<std::uint64_t>(
                        metrics.total_edges),
                    static_cast<std::uint64_t>(
                        options.destination_entry_bytes),
                    "HyTMCostModel: filter transfer byte count overflow");

            /*
             * The active vertex count and active edge count follow the
             * project's ActivityTracker definition: every outgoing edge of
             * an active source vertex belongs to the active edge set.
             */
            for (VertexId vertex = partition.vertex_begin();
                 vertex < partition.vertex_end();
                 ++vertex)
            {
                if (!activity.is_active(vertex))
                {
                    continue;
                }

                ++metrics.active_vertices;

                const OffsetType degree =
                    graph.out_degree(vertex);

                if (degree >
                    std::numeric_limits<OffsetType>::max() -
                        metrics.active_edges)
                {
                    throw std::overflow_error(
                        "HyTMCostModel: active edge count overflow");
                }

                metrics.active_edges += degree;

                const std::uint64_t zero_copy_requests =
                    zero_copy_vertex_requests(
                        graph,
                        vertex,
                        options);

                metrics.zero_copy_memory_requests =
                    checked_add(
                        metrics.zero_copy_memory_requests,
                        zero_copy_requests,
                        "HyTMCostModel: zero-copy request count overflow");

                const std::uint64_t alignment =
                    zero_copy_alignment_overhead(
                        graph,
                        vertex,
                        options);

                metrics.zero_copy_alignment_overhead =
                    checked_add(
                        metrics.zero_copy_alignment_overhead,
                        alignment,
                        "HyTMCostModel: zero-copy alignment count overflow");
            }

            /*
             * ExpTM-Compaction transfer volume:
             *
             *   sum Do(v) * d1 + |Ai| * d2
             *
             * where d2 is the vertex-index entry size.
             */
            const std::uint64_t active_edge_bytes =
                checked_multiply(
                    static_cast<std::uint64_t>(
                        metrics.active_edges),
                    static_cast<std::uint64_t>(
                        options.destination_entry_bytes),
                    "HyTMCostModel: compaction edge byte count overflow");

            const std::uint64_t active_index_bytes =
                checked_multiply(
                    static_cast<std::uint64_t>(
                        metrics.active_vertices),
                    static_cast<std::uint64_t>(
                        options.vertex_index_bytes),
                    "HyTMCostModel: compaction index byte count overflow");

            metrics.compaction_bytes =
                checked_add(
                    active_edge_bytes,
                    active_index_bytes,
                    "HyTMCostModel: compaction transfer byte count "
                    "overflow");

            metrics.zero_copy_total_requests =
                checked_add(
                    metrics.zero_copy_memory_requests,
                    metrics.zero_copy_alignment_overhead,
                    "HyTMCostModel: zero-copy total request count "
                    "overflow");

            return metrics;
        }

        double zero_copy_rtt(
            const HyTMPartitionMetrics &metrics,
            const HyTMCostModelOptions &options)
        {
            /*
             * Paper:
             *
             * RTTzc = gamma * RTT
             *       + (1 - gamma)
             *         * active_edge_ratio
             *         * RTT
             *
             * When a partition has no edges, the active-edge proportion is
             * not defined by the paper's formula. For the reference model we
             * use zero as the payload-dependent component, leaving the
             * fixed gamma * RTT component.
             */
            if (metrics.total_edges == 0U)
            {
                return options.gamma * options.rtt;
            }

            const double active_edge_ratio =
                static_cast<double>(metrics.active_edges) /
                static_cast<double>(metrics.total_edges);

            return options.gamma * options.rtt +
                   (1.0 - options.gamma) *
                       active_edge_ratio *
                       options.rtt;
        }

        double transfer_cost(
            std::uint64_t bytes,
            const HyTMCostModelOptions &options)
        {
            const std::uint64_t tlps =
                saturated_tlp_count(bytes, options);

            return static_cast<double>(tlps) * options.rtt;
        }

    } // namespace

    HyTMCostModel::HyTMCostModel(
        HyTMCostModelOptions options)
        : options_(std::move(options))
    {
        validate_options(options_);
    }

    const HyTMCostModelOptions &
    HyTMCostModel::options() const noexcept
    {
        return options_;
    }

    HyTMPartitionCosts HyTMCostModel::evaluate_partition(
        const graph::CSRGraph &graph,
        const graph::LogicalPartition &partition,
        const graph::ActivityTracker &activity,
        std::size_t partition_index) const
    {
        validate_options(options_);
        validate_activity(graph, activity);
        validate_partition(graph, partition);

        HyTMPartitionCosts result;

        result.metrics =
            collect_metrics(
                graph,
                partition,
                activity,
                options_,
                partition_index);

        /*
         * Formula (1):
         *
         *   Tefi =
         *       ceil(
         *           sum Do(v) * d1 / m / MR
         *       ) * RTT
         *
         * For ExpTM-Filter the complete partition is transferred, so
         * partition.edge_count() * d1 is the modeled transfer volume.
         */
        result.filter_cost =
            transfer_cost(
                result.metrics.filter_transfer_bytes,
                options_);

        /*
         * Formula (2):
         *
         *   Teci =
         *       ceil(
         *           (
         *             sum Do(v) * d1 + |Ai| * d2
         *           ) / m / MR
         *       ) * RTT
         *
         *       + transfer_volume / Thptcpt
         *
         * The throughput term is retained because this reproduction exposes
         * it as a configurable model parameter.
         */
        const std::uint64_t compaction_tlps =
            saturated_tlp_count(
                result.metrics.compaction_bytes,
                options_);

        const double compaction_transfer_cost =
            static_cast<double>(compaction_tlps) *
            options_.rtt;

        const double compaction_cpu_cost =
            static_cast<double>(
                result.metrics.compaction_bytes) /
            options_.cpu_compaction_throughput_bytes_per_second;

        result.compaction_cost =
            compaction_transfer_cost +
            compaction_cpu_cost;

        /*
         * Formula (3):
         *
         *   Tizi =
         *       ceil(
         *           sum(
         *             ceil(Do(v) * d1 / m) + am(v)
         *           ) / MR
         *       )
         *       * RTTzc
         */
        result.zero_copy_rtt =
            zero_copy_rtt(
                result.metrics,
                options_);

        const std::uint64_t zero_copy_tlps =
            ceil_div(
                result.metrics.zero_copy_total_requests,
                static_cast<std::uint64_t>(
                    options_.max_requests_per_tlp));

        result.zero_copy_cost =
            static_cast<double>(zero_copy_tlps) *
            result.zero_copy_rtt;

        result.selected_engine =
            select_engine(
                result.filter_cost,
                result.compaction_cost,
                result.zero_copy_cost,
                options_.alpha,
                options_.beta);

        return result;
    }

    std::vector<HyTMPartitionCosts>
    HyTMCostModel::evaluate_partitions(
        const graph::CSRGraph &graph,
        const std::vector<graph::LogicalPartition> &partitions,
        const graph::ActivityTracker &activity) const
    {
        validate_options(options_);
        validate_activity(graph, activity);

        /*
         * Preserve the logical partition ordering exactly. Each partition
         * is evaluated independently, matching the paper's observation that
         * the cost computation between partitions is independent.
         */
        std::vector<HyTMPartitionCosts> results;
        results.reserve(partitions.size());

        for (std::size_t index = 0U;
             index < partitions.size();
             ++index)
        {
            results.push_back(
                evaluate_partition(
                    graph,
                    partitions[index],
                    activity,
                    index));
        }

        return results;
    }

    TransferEngine HyTMCostModel::select_engine(
        double filter_cost,
        double compaction_cost,
        double zero_copy_cost,
        double alpha,
        double beta)
    {
        if (!std::isfinite(filter_cost) ||
            !std::isfinite(compaction_cost) ||
            !std::isfinite(zero_copy_cost))
        {
            throw std::invalid_argument(
                "HyTMCostModel::select_engine: costs must be finite");
        }

        if (filter_cost < 0.0 ||
            compaction_cost < 0.0 ||
            zero_copy_cost < 0.0)
        {
            throw std::invalid_argument(
                "HyTMCostModel::select_engine: costs must be "
                "non-negative");
        }

        if (!std::isfinite(alpha) || alpha <= 0.0)
        {
            throw std::invalid_argument(
                "HyTMCostModel::select_engine: alpha must be "
                "finite and greater than zero");
        }

        if (!std::isfinite(beta) || beta <= 0.0)
        {
            throw std::invalid_argument(
                "HyTMCostModel::select_engine: beta must be "
                "finite and greater than zero");
        }

        /*
         * Algorithm 1, lines 4-13:
         *
         *   if Teci < alpha * Tefi
         *      and Teci < beta * Tizi
         *       -> ExpTM-C
         *
         *   else if Tefi < Tizi
         *       -> ExpTM-F
         *
         *   else
         *       -> ImpTM-ZC
         *
         * The comparisons are intentionally strict. Equality therefore
         * falls through to the later comparison exactly as in the paper.
         */
        if (compaction_cost < alpha * filter_cost &&
            compaction_cost < beta * zero_copy_cost)
        {
            return TransferEngine::ExpTMCompaction;
        }

        if (filter_cost < zero_copy_cost)
        {
            return TransferEngine::ExpTMFilter;
        }

        return TransferEngine::ImpTMZeroCopy;
    }

} // namespace hytgraph::transfer