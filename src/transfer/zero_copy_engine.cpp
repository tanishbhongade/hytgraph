#include "transfer/zero_copy_engine.hpp"

#include <chrono>
#include <limits>
#include <stdexcept>
#include <utility>

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

                /*
                 * A CSR-preserving partition must begin/end on CSR row
                 * boundaries. The logical partition's edge range must
                 * therefore correspond to the graph's row offsets.
                 */
                if (graph.row_offsets()[static_cast<std::size_t>(partition.vertex_begin())] !=
                    partition.edge_begin())
                {
                    throw std::invalid_argument(
                        "logical partition edge_begin is not a CSR row boundary");
                }

                if (graph.row_offsets()[static_cast<std::size_t>(partition.vertex_end())] !=
                    partition.edge_end())
                {
                    throw std::invalid_argument(
                        "logical partition edge_end is not a CSR row boundary");
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

        void validate_options(const ZeroCopyOptions &options)
        {
            if (options.request_payload_bytes == 0U)
            {
                throw std::invalid_argument(
                    "zero-copy request payload size must be greater than zero");
            }

            if (options.max_requests_per_tlp == 0U)
            {
                throw std::invalid_argument(
                    "zero-copy maximum requests per TLP must be greater than zero");
            }

            if (options.alignment_bytes == 0U)
            {
                throw std::invalid_argument(
                    "zero-copy alignment size must be greater than zero");
            }
        }

        std::uint64_t ceil_div(
            std::uint64_t numerator,
            std::uint64_t denominator)
        {
            if (denominator == 0U)
            {
                throw std::invalid_argument(
                    "zero-copy division denominator must be greater than zero");
            }

            return numerator / denominator +
                   ((numerator % denominator) != 0U ? 1U : 0U);
        }

        /*
         * Phase 7 reference/model address.
         *
         * The paper's implementation can determine alignment from the
         * physical position of neighbor data. The current portable CSR
         * interface does not expose such an address.
         *
         * We therefore model the neighbor array as a contiguous byte
         * sequence beginning at byte offset zero. The adjacency list for
         * vertex v begins at:
         *
         *     row_offsets[v] * sizeof(vertex_id)
         *
         * This is deliberately a logical address, not a physical host
         * address.
         */
        std::uint64_t logical_neighbor_byte_offset(
            const graph::CSRGraph &graph,
            graph::CSRGraph::vertex_id vertex)
        {
            constexpr std::uint64_t vertex_bytes =
                sizeof(graph::CSRGraph::vertex_id);

            const auto edge_offset =
                graph.row_offsets()[static_cast<std::size_t>(vertex)];

            if (edge_offset >
                std::numeric_limits<std::uint64_t>::max() / vertex_bytes)
            {
                throw std::overflow_error(
                    "zero-copy logical neighbor byte offset overflow");
            }

            return edge_offset * vertex_bytes;
        }

        ZeroCopyVertexMetrics calculate_vertex_metrics(
            const graph::CSRGraph &graph,
            graph::CSRGraph::vertex_id vertex,
            const ZeroCopyOptions &options)
        {
            ZeroCopyVertexMetrics metrics;
            metrics.vertex = vertex;
            metrics.degree = graph.out_degree(vertex);

            const std::uint64_t payload_bytes =
                static_cast<std::uint64_t>(metrics.degree) *
                static_cast<std::uint64_t>(
                    sizeof(graph::CSRGraph::vertex_id));

            /*
             * Formula corresponding to the paper's request model:
             *
             *     ceil(Do(v) * d1 / m) + am(v)
             *
             * where m is the maximum request payload.
             *
             * For the reference implementation d1 is the CSR vertex-ID
             * storage size.
             */
            metrics.memory_requests =
                ceil_div(
                    payload_bytes,
                    static_cast<std::uint64_t>(
                        options.request_payload_bytes));

            const std::uint64_t byte_offset =
                logical_neighbor_byte_offset(graph, vertex);

            /*
             * Empty adjacency lists have no memory transaction and
             * consequently do not incur alignment overhead.
             */
            if (metrics.degree != 0U &&
                (byte_offset %
                 static_cast<std::uint64_t>(
                     options.alignment_bytes)) != 0U)
            {
                metrics.alignment_overhead = 1U;
            }

            return metrics;
        }

    } // namespace

    ImpTMZeroCopy::ImpTMZeroCopy(
        ZeroCopyOptions options) noexcept
        : options_(std::move(options))
    {
    }

    const ZeroCopyOptions &
    ImpTMZeroCopy::options() const noexcept
    {
        return options_;
    }

    ZeroCopyResult ImpTMZeroCopy::prepare(
        const graph::CSRGraph &graph,
        const std::vector<graph::LogicalPartition> &partitions,
        const graph::ActivityTracker &activity) const
    {
        const auto start = std::chrono::steady_clock::now();

        validate_options(options_);

        if (activity.vertex_count() != graph.num_vertices())
        {
            throw std::invalid_argument(
                "activity tracker vertex count does not match graph");
        }

        validate_partitions(graph, partitions);

        ZeroCopyResult result;

        /*
         * The current Phase 7 source is the portable reference/model path.
         *
         * Actual CUDA host-memory mapping is intentionally not claimed here:
         * this translation unit is compiled as part of hytgraph_runtime, which
         * is currently a C++ target. The CUDA-specific execution integration
         * belongs at the point where the project introduces the corresponding
         * CUDA runtime path.
         */
        result.mode = ZeroCopyMode::Modeled;
        result.host_memory_mapped = false;

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

        result.partitions.reserve(partitions.size());

        for (std::size_t index = 0U;
             index < partitions.size();
             ++index)
        {
            const graph::LogicalPartition &partition =
                partitions[index];

            const auto &activity_stats = statistics[index];

            ZeroCopyPartition zero_copy_partition;

            zero_copy_partition.partition_index = index;
            zero_copy_partition.vertex_begin =
                partition.vertex_begin();
            zero_copy_partition.vertex_end =
                partition.vertex_end();

            if (activity_stats.active_vertices == 0U)
            {
                result.partitions.push_back(
                    std::move(zero_copy_partition));

                continue;
            }

            zero_copy_partition.active_vertices.reserve(
                static_cast<std::size_t>(
                    activity_stats.active_vertices));

            zero_copy_partition.vertex_metrics.reserve(
                static_cast<std::size_t>(
                    activity_stats.active_vertices));

            for (graph::CSRGraph::vertex_id vertex =
                     partition.vertex_begin();
                 vertex < partition.vertex_end();
                 ++vertex)
            {
                if (!activity.is_active(vertex))
                {
                    continue;
                }

                zero_copy_partition.active_vertices.push_back(vertex);

                const ZeroCopyVertexMetrics metrics =
                    calculate_vertex_metrics(
                        graph,
                        vertex,
                        options_);

                zero_copy_partition.active_edge_count +=
                    metrics.degree;

                zero_copy_partition.memory_request_count +=
                    metrics.memory_requests;

                zero_copy_partition.alignment_overhead_count +=
                    metrics.alignment_overhead;

                zero_copy_partition.vertex_metrics.push_back(
                    metrics);
            }

            // zero_copy_partition.modeled_tlp_count =
            //     ceil_div(
            //         zero_copy_partition.memory_request_count,
            //         static_cast<std::uint64_t>(
            //             options_.max_requests_per_tlp));
            zero_copy_partition.modeled_tlp_count = 0;

            if (!zero_copy_partition.active_vertices.empty())
            {
                ++result.active_partition_count;
            }

            result.active_vertex_count +=
                static_cast<graph::CSRGraph::offset_type>(
                    zero_copy_partition.active_vertices.size());

            result.active_edge_count +=
                zero_copy_partition.active_edge_count;

            result.memory_request_count +=
                zero_copy_partition.memory_request_count;

            result.alignment_overhead_count +=
                zero_copy_partition.alignment_overhead_count;

            result.modeled_tlp_count +=
                zero_copy_partition.modeled_tlp_count;

            result.partitions.push_back(
                std::move(zero_copy_partition));
        }

        const auto end = std::chrono::steady_clock::now();
        result.modeled_tlp_count =
            ceil_div(
                result.memory_request_count,
                static_cast<std::uint64_t>(
                    options_.max_requests_per_tlp));
        result.preparation_seconds =
            std::chrono::duration<double>(end - start).count();

        return result;
    }

} // namespace hytgraph::transfer