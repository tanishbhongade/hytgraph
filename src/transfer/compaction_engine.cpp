#include "transfer/compaction_engine.hpp"

#include <chrono>
#include <limits>
#include <stdexcept>
#include <utility>

namespace hytgraph::transfer
{

    namespace
    {

        using VertexId = graph::CSRGraph::vertex_id;
        using OffsetType = graph::CSRGraph::offset_type;

        void validate_partition(
            const graph::CSRGraph &graph,
            const graph::LogicalPartition &partition)
        {
            const VertexId vertex_count =
                static_cast<VertexId>(graph.num_vertices());

            if (partition.vertex_begin() > partition.vertex_end())
            {
                throw std::invalid_argument(
                    "ExpTMCompaction: partition vertex range is invalid");
            }

            if (partition.vertex_end() > vertex_count)
            {
                throw std::invalid_argument(
                    "ExpTMCompaction: partition vertex range exceeds graph");
            }

            if (partition.edge_begin() > partition.edge_end())
            {
                throw std::invalid_argument(
                    "ExpTMCompaction: partition edge range is invalid");
            }

            const OffsetType graph_edge_count = graph.num_edges();

            if (partition.edge_end() > graph_edge_count)
            {
                throw std::invalid_argument(
                    "ExpTMCompaction: partition edge range exceeds graph");
            }

            // Logical partitions are required to correspond to complete
            // CSR rows. This is an invariant of LogicalPartitioner.
            if (partition.vertex_begin() < vertex_count)
            {
                const auto [expected_begin, unused_end] =
                    graph.neighbor_range(partition.vertex_begin());

                (void)unused_end;

                if (partition.edge_begin() != expected_begin)
                {
                    throw std::invalid_argument(
                        "ExpTMCompaction: partition edge_begin does not "
                        "match its first CSR row");
                }
            }

            if (partition.vertex_end() > partition.vertex_begin())
            {
                const auto [unused_begin, expected_end] =
                    graph.neighbor_range(partition.vertex_end() - 1U);

                (void)unused_begin;

                if (partition.edge_end() != expected_end)
                {
                    throw std::invalid_argument(
                        "ExpTMCompaction: partition edge_end does not "
                        "match its final CSR row");
                }
            }
        }

        void validate_activity(
            const graph::CSRGraph &graph,
            const graph::ActivityTracker &activity)
        {
            if (activity.vertex_count() != graph.num_vertices())
            {
                throw std::invalid_argument(
                    "ExpTMCompaction: activity tracker vertex count does "
                    "not match graph");
            }
        }
    } // namespace

    graph::CSRGraph::offset_type
    CompactedPartition::active_edge_count() const noexcept
    {
        if (neighbors.size() >
            static_cast<std::size_t>(
                std::numeric_limits<graph::CSRGraph::offset_type>::max()))
        {
            return std::numeric_limits<graph::CSRGraph::offset_type>::max();
        }

        return static_cast<graph::CSRGraph::offset_type>(neighbors.size());
    }

    std::size_t
    CompactedPartition::active_vertex_count() const noexcept
    {
        return active_vertices.size();
    }

    std::size_t
    CompactedPartition::neighbor_bytes() const noexcept
    {
        return neighbors.size() * sizeof(graph::CSRGraph::vertex_id);
    }

    std::size_t
    CompactedPartition::index_bytes() const noexcept
    {
        return neighbor_index.size() *
               sizeof(graph::CSRGraph::offset_type);
    }

    std::size_t
    CompactedPartition::total_bytes() const noexcept
    {
        return neighbor_bytes() + index_bytes();
    }

    std::size_t
    CompactionResult::compacted_partition_count() const noexcept
    {
        return partitions.size();
    }

    graph::CSRGraph::offset_type
    CompactionResult::active_edge_count() const noexcept
    {
        graph::CSRGraph::offset_type total = 0;

        for (const auto &partition : partitions)
        {
            const auto count = partition.active_edge_count();

            if (count >
                std::numeric_limits<graph::CSRGraph::offset_type>::max() -
                    total)
            {
                return std::numeric_limits<
                    graph::CSRGraph::offset_type>::max();
            }

            total += count;
        }

        return total;
    }

    graph::CSRGraph::offset_type
    CompactionResult::active_vertex_count() const noexcept
    {
        graph::CSRGraph::offset_type total = 0;

        for (const auto &partition : partitions)
        {
            const std::size_t count = partition.active_vertex_count();

            if (count >
                static_cast<std::size_t>(
                    std::numeric_limits<
                        graph::CSRGraph::offset_type>::max() -
                    total))
            {
                return std::numeric_limits<
                    graph::CSRGraph::offset_type>::max();
            }

            total +=
                static_cast<graph::CSRGraph::offset_type>(count);
        }

        return total;
    }

    std::size_t
    CompactionResult::neighbor_bytes() const noexcept
    {
        std::size_t total = 0;

        for (const auto &partition : partitions)
        {
            const std::size_t bytes = partition.neighbor_bytes();

            if (bytes > std::numeric_limits<std::size_t>::max() - total)
            {
                return std::numeric_limits<std::size_t>::max();
            }

            total += bytes;
        }

        return total;
    }

    std::size_t
    CompactionResult::index_bytes() const noexcept
    {
        std::size_t total = 0;

        for (const auto &partition : partitions)
        {
            const std::size_t bytes = partition.index_bytes();

            if (bytes > std::numeric_limits<std::size_t>::max() - total)
            {
                return std::numeric_limits<std::size_t>::max();
            }

            total += bytes;
        }

        return total;
    }

    std::size_t
    CompactionResult::total_bytes() const noexcept
    {
        const std::size_t neighbors = neighbor_bytes();
        const std::size_t index = index_bytes();

        if (index > std::numeric_limits<std::size_t>::max() - neighbors)
        {
            return std::numeric_limits<std::size_t>::max();
        }

        return neighbors + index;
    }

    ExpTMCompaction::ExpTMCompaction(bool enabled) noexcept
        : enabled_(enabled)
    {
    }

    bool ExpTMCompaction::enabled() const noexcept
    {
        return enabled_;
    }

    CompactionResult ExpTMCompaction::compact(
        const graph::CSRGraph &graph,
        const std::vector<graph::LogicalPartition> &partitions,
        const graph::ActivityTracker &activity) const
    {
        CompactionResult result;

        if (!enabled_)
        {
            return result;
        }

        validate_activity(graph, activity);

        /*
         * The timing covers the CPU-side compaction work only.
         *
         * In particular, it does not include:
         *
         *   - cudaMemcpy
         *   - GPU computation
         *   - CUDA stream scheduling
         *   - later HyTM selection
         *
         * This keeps the Phase 6 measurement separate from transfer
         * execution, as required by the phase definition.
         */
        const auto start = std::chrono::steady_clock::now();

        result.partitions.reserve(partitions.size());

        for (std::size_t partition_index = 0;
             partition_index < partitions.size();
             ++partition_index)
        {
            const graph::LogicalPartition &partition =
                partitions[partition_index];

            validate_partition(graph, partition);

            CompactedPartition compacted;
            compacted.partition_index = partition_index;
            compacted.vertex_begin = partition.vertex_begin();
            compacted.vertex_end = partition.vertex_end();

            /*
             * First collect active source vertices.
             *
             * ActivityTracker::active_vertices() is already specified to
             * return ascending vertex-ID order. Filtering it by the
             * partition range therefore preserves deterministic CSR/source
             * order.
             */
            const std::vector<VertexId> active_vertices =
                activity.active_vertices();

            for (const VertexId vertex : active_vertices)
            {
                if (vertex < partition.vertex_begin())
                {
                    continue;
                }

                if (vertex >= partition.vertex_end())
                {
                    break;
                }

                compacted.active_vertices.push_back(vertex);
            }

            // ExpTM-Compaction does not transfer an inactive partition.
            if (compacted.active_vertices.empty())
            {
                continue;
            }

            /*
             * One index entry is needed before the first active vertex,
             * followed by one terminal entry for every active vertex.
             */
            compacted.neighbor_index.reserve(
                compacted.active_vertices.size() + 1U);

            compacted.neighbor_index.push_back(0);

            /*
             * Reserve the known active-edge payload where possible.
             *
             * Activity is defined in this project as all outgoing edges
             * belonging to active source vertices.
             */
            std::size_t estimated_edge_count = 0;

            for (const VertexId vertex :
                 compacted.active_vertices)
            {
                const auto [begin, end] =
                    graph.neighbor_range(vertex);

                const std::size_t degree =
                    static_cast<std::size_t>(end - begin);

                if (degree >
                    std::numeric_limits<std::size_t>::max() -
                        estimated_edge_count)
                {
                    throw std::overflow_error(
                        "ExpTMCompaction: active-edge count overflow");
                }

                estimated_edge_count += degree;
            }

            compacted.neighbors.reserve(estimated_edge_count);

            /*
             * Compact each active source row.
             *
             * The destination IDs are copied exactly as they occur in the
             * source CSR graph. No vertex renumbering or neighbor sorting is
             * introduced by this phase.
             */
            OffsetType compacted_offset = 0;

            for (const VertexId vertex :
                 compacted.active_vertices)
            {
                const auto [begin, end] =
                    graph.neighbor_range(vertex);

                for (OffsetType edge = begin; edge < end; ++edge)
                {
                    compacted.neighbors.push_back(
                        graph.neighbor_at(edge));
                }

                const OffsetType degree = end - begin;

                if (degree >
                    std::numeric_limits<OffsetType>::max() -
                        compacted_offset)
                {
                    throw std::overflow_error(
                        "ExpTMCompaction: compacted index offset overflow");
                }

                compacted_offset += degree;
                compacted.neighbor_index.push_back(compacted_offset);
            }

            /*
             * Correctness invariant:
             *
             * The terminal compressed offset must equal the number of
             * compacted destination IDs.
             */
            if (compacted.neighbor_index.back() !=
                static_cast<OffsetType>(compacted.neighbors.size()))
            {
                throw std::logic_error(
                    "ExpTMCompaction: compressed neighbor index is "
                    "inconsistent with compacted neighbors");
            }

            result.partitions.push_back(std::move(compacted));
        }

        const auto finish = std::chrono::steady_clock::now();

        result.compaction_seconds =
            std::chrono::duration<double>(finish - start).count();

        return result;
    }

    std::vector<graph::CSRGraph::vertex_id>
    ExpTMCompaction::expanded_neighbors(
        const CompactedPartition &partition)
    {
        if (partition.neighbor_index.size() !=
            partition.active_vertices.size() + 1U)
        {
            throw std::invalid_argument(
                "ExpTMCompaction::expanded_neighbors: compressed index "
                "size does not match active vertex count");
        }

        if (partition.neighbor_index.empty())
        {
            if (!partition.neighbors.empty())
            {
                throw std::invalid_argument(
                    "ExpTMCompaction::expanded_neighbors: empty index "
                    "cannot describe neighbors");
            }

            return {};
        }

        if (partition.neighbor_index.front() != 0U)
        {
            throw std::invalid_argument(
                "ExpTMCompaction::expanded_neighbors: index must start "
                "at zero");
        }

        const OffsetType neighbor_count =
            static_cast<OffsetType>(partition.neighbors.size());

        if (partition.neighbor_index.back() != neighbor_count)
        {
            throw std::invalid_argument(
                "ExpTMCompaction::expanded_neighbors: terminal index "
                "does not match neighbor count");
        }

        for (std::size_t i = 1;
             i < partition.neighbor_index.size();
             ++i)
        {
            if (partition.neighbor_index[i] <
                partition.neighbor_index[i - 1U])
            {
                throw std::invalid_argument(
                    "ExpTMCompaction::expanded_neighbors: index is not "
                    "monotonic");
            }
        }

        /*
         * This function returns the compacted edge stream. The source
         * vertex associated with each range is represented by the
         * corresponding entry in active_vertices.
         *
         * It intentionally returns only destinations because that is the
         * compacted neighbor payload represented by this class.
         */
        return partition.neighbors;
    }

} // namespace hytgraph::transfer