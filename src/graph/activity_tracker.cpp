#include "graph/activity_tracker.hpp"

#include <stdexcept>
#include <string>

namespace hytgraph::graph
{

    ActivityTracker::ActivityTracker(count_type vertex_count)
        : vertex_count_(vertex_count),
          active_(static_cast<std::size_t>(vertex_count), 0U),
          active_vertex_count_(0U)
    {
    }

    ActivityTracker::count_type
    ActivityTracker::vertex_count() const noexcept
    {
        return vertex_count_;
    }

    void ActivityTracker::clear() noexcept
    {
        std::fill(active_.begin(), active_.end(), 0U);
        active_vertex_count_ = 0U;
    }

    void ActivityTracker::set_active(vertex_id vertex, bool active)
    {
        validate_vertex(vertex);

        const std::size_t index = static_cast<std::size_t>(vertex);
        const bool currently_active = active_[index] != 0U;

        if (currently_active == active)
        {
            return;
        }

        active_[index] = active ? 1U : 0U;

        if (active)
        {
            ++active_vertex_count_;
        }
        else
        {
            --active_vertex_count_;
        }
    }

    bool ActivityTracker::is_active(vertex_id vertex) const
    {
        validate_vertex(vertex);

        return active_[static_cast<std::size_t>(vertex)] != 0U;
    }

    void ActivityTracker::set_active_vertices(
        const std::vector<vertex_id> &vertices)
    {
        clear();

        for (const vertex_id vertex : vertices)
        {
            set_active(vertex, true);
        }
    }

    std::vector<ActivityTracker::vertex_id>
    ActivityTracker::active_vertices() const
    {
        std::vector<vertex_id> vertices;
        vertices.reserve(static_cast<std::size_t>(active_vertex_count_));

        for (count_type vertex = 0U; vertex < vertex_count_; ++vertex)
        {
            const auto vertex_id_value = static_cast<vertex_id>(vertex);

            if (active_[static_cast<std::size_t>(vertex)] != 0U)
            {
                vertices.push_back(vertex_id_value);
            }
        }

        return vertices;
    }

    ActivityTracker::count_type
    ActivityTracker::active_vertex_count() const noexcept
    {
        return active_vertex_count_;
    }

    ActivityTracker::count_type
    ActivityTracker::active_edge_count(const CSRGraph &graph) const
    {
        validate_graph(graph);

        count_type active_edges = 0U;

        for (count_type vertex = 0U; vertex < vertex_count_; ++vertex)
        {
            if (active_[static_cast<std::size_t>(vertex)] == 0U)
            {
                continue;
            }

            active_edges += graph.out_degree(
                static_cast<vertex_id>(vertex));
        }

        return active_edges;
    }

    std::vector<ActivityTracker::PartitionActivity>
    ActivityTracker::partition_statistics(
        const CSRGraph &graph,
        const std::vector<VertexRange> &partitions) const
    {
        validate_graph(graph);

        std::vector<PartitionActivity> statistics;
        statistics.reserve(partitions.size());

        for (const VertexRange &range : partitions)
        {
            validate_range(range);

            PartitionActivity activity;

            for (count_type vertex = range.begin;
                 vertex < range.end;
                 ++vertex)
            {
                const auto vertex_id_value =
                    static_cast<vertex_id>(vertex);

                const count_type out_degree =
                    graph.out_degree(vertex_id_value);

                activity.total_edges += out_degree;

                if (active_[static_cast<std::size_t>(vertex)] != 0U)
                {
                    ++activity.active_vertices;
                    activity.active_edges += out_degree;
                }
            }

            statistics.push_back(activity);
        }

        return statistics;
    }

    void ActivityTracker::validate_vertex(vertex_id vertex) const
    {
        if (static_cast<count_type>(vertex) >= vertex_count_)
        {
            throw std::out_of_range(
                "activity vertex is out of range");
        }
    }

    void ActivityTracker::validate_graph(const CSRGraph &graph) const
    {
        if (graph.num_vertices() != vertex_count_)
        {
            throw std::invalid_argument(
                "activity tracker vertex count does not match graph");
        }
    }

    void ActivityTracker::validate_range(const VertexRange &range) const
    {
        if (static_cast<count_type>(range.begin) >
            static_cast<count_type>(range.end))
        {
            throw std::invalid_argument(
                "activity partition range has begin greater than end");
        }

        if (static_cast<count_type>(range.end) > vertex_count_)
        {
            throw std::out_of_range(
                "activity partition range exceeds vertex count");
        }
    }

} // namespace hytgraph::graph