#include "algorithms/pagerank.hpp"
#include "algorithms/sssp.hpp"
#include "graph/activity_tracker.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace
{

    void expect(bool condition, const std::string &message)
    {
        if (!condition)
        {
            throw std::runtime_error("FAIL: " + message);
        }
    }

    void test_pagerank_cycle()
    {
        using hytgraph::algorithms::pagerank_cpu;
        using hytgraph::algorithms::PageRankOptions;
        using hytgraph::graph::CSRGraph;

        // 0 -> 1 -> 2 -> 0.
        CSRGraph graph(
            3,
            {0, 1, 2, 3},
            {1, 2, 0});

        PageRankOptions options;
        options.max_iterations = 100;
        options.tolerance = 1.0e-6F;

        const auto result = pagerank_cpu(graph, options);

        expect(result.converged, "PageRank cycle converges");
        expect(result.ranks.size() == 3, "PageRank result size");
        for (const float rank : result.ranks)
        {
            expect(std::fabs(rank - (1.0F / 3.0F)) <= 1.0e-5F,
                   "PageRank cycle converges to uniform rank");
        }
    }

    void test_pagerank_dangling_vertex()
    {
        using hytgraph::algorithms::pagerank_cpu;
        using hytgraph::algorithms::PageRankOptions;
        using hytgraph::graph::CSRGraph;

        // 0 <-> 1, with vertex 2 dangling.
        CSRGraph graph(
            3,
            {0, 1, 2, 2},
            {1, 0});

        PageRankOptions options;
        options.max_iterations = 200;
        options.tolerance = 1.0e-6F;

        const auto result = pagerank_cpu(graph, options);
        expect(result.converged, "PageRank with dangling vertex converges");

        float sum = 0.0F;
        for (const float rank : result.ranks)
        {
            sum += rank;
        }
        expect(std::fabs(sum - 1.0F) <= 1.0e-5F,
               "PageRank preserves total probability mass");
    }

    void test_pagerank_invalid_options()
    {
        using hytgraph::algorithms::pagerank_cpu;
        using hytgraph::algorithms::PageRankOptions;
        using hytgraph::graph::CSRGraph;

        CSRGraph graph(1, {0, 0}, {});

        PageRankOptions options;
        options.damping_factor = 1.0F;

        bool threw = false;
        try
        {
            (void)pagerank_cpu(graph, options);
        }
        catch (const std::invalid_argument &)
        {
            threw = true;
        }
        expect(threw, "PageRank rejects invalid damping factor");
    }

    void test_sssp_weighted()
    {
        using hytgraph::algorithms::sssp_cpu;
        using hytgraph::algorithms::SSSPOptions;
        using hytgraph::graph::CSRGraph;

        // 0 -> 1 (1), 0 -> 2 (4), 1 -> 2 (2),
        // 1 -> 3 (6), 2 -> 3 (1).
        CSRGraph graph(
            4,
            {0, 2, 4, 5, 5},
            {1, 2, 2, 3, 3},
            {1.0F, 4.0F, 2.0F, 6.0F, 1.0F});

        SSSPOptions options;
        options.source = 0;

        const auto result = sssp_cpu(graph, options);

        expect(result.converged, "weighted SSSP converges");
        expect(result.distances.size() == 4, "SSSP result size");
        expect(std::fabs(result.distances[0] - 0.0F) <= 1.0e-6F,
               "source distance");
        expect(std::fabs(result.distances[1] - 1.0F) <= 1.0e-6F,
               "distance to vertex 1");
        expect(std::fabs(result.distances[2] - 3.0F) <= 1.0e-6F,
               "distance to vertex 2");
        expect(std::fabs(result.distances[3] - 4.0F) <= 1.0e-6F,
               "distance to vertex 3");
    }

    void test_sssp_unweighted_and_unreachable()
    {
        using hytgraph::algorithms::sssp_cpu;
        using hytgraph::algorithms::SSSPOptions;
        using hytgraph::graph::CSRGraph;

        // 0 -> 1 -> 2, vertex 3 unreachable.
        CSRGraph graph(
            4,
            {0, 1, 2, 2, 2},
            {1, 2});

        SSSPOptions options;
        options.source = 0;

        const auto result = sssp_cpu(graph, options);

        expect(result.converged, "unweighted SSSP converges");
        expect(std::fabs(result.distances[0] - 0.0F) <= 1.0e-6F,
               "unweighted source distance");
        expect(std::fabs(result.distances[1] - 1.0F) <= 1.0e-6F,
               "unweighted distance to vertex 1");
        expect(std::fabs(result.distances[2] - 2.0F) <= 1.0e-6F,
               "unweighted distance to vertex 2");
        expect(std::isinf(result.distances[3]),
               "unreachable vertex remains infinite");
    }

    void test_sssp_rejects_negative_weights()
    {
        using hytgraph::algorithms::sssp_cpu;
        using hytgraph::algorithms::SSSPOptions;
        using hytgraph::graph::CSRGraph;

        CSRGraph graph(
            2,
            {0, 1, 1},
            {1},
            {-1.0F});

        bool threw = false;
        try
        {
            (void)sssp_cpu(graph, SSSPOptions{});
        }
        catch (const std::invalid_argument &)
        {
            threw = true;
        }

        expect(threw, "SSSP rejects negative edge weights");
    }

    void test_sssp_rejects_invalid_source()
    {
        using hytgraph::algorithms::sssp_cpu;
        using hytgraph::algorithms::SSSPOptions;
        using hytgraph::graph::CSRGraph;

        CSRGraph graph(2, {0, 0, 0}, {});

        SSSPOptions options;
        options.source = 2;

        bool threw = false;
        try
        {
            (void)sssp_cpu(graph, options);
        }
        catch (const std::invalid_argument &)
        {
            threw = true;
        }

        expect(threw, "SSSP rejects out-of-range source");
    }

    void test_activity_tracker_basic()
    {
        using hytgraph::graph::ActivityTracker;
        using hytgraph::graph::CSRGraph;

        // Graph:
        //
        // 0 -> 1, 2
        // 1 -> 2
        // 2 -> 0
        // 3 -> nothing
        //
        // Out-degrees:
        // 0: 2
        // 1: 1
        // 2: 1
        // 3: 0

        CSRGraph graph(
            4,
            {0, 2, 3, 4, 4},
            {1, 2, 2, 0});

        ActivityTracker activity(graph.num_vertices());

        expect(activity.vertex_count() == 4,
               "activity tracker vertex count");

        expect(activity.active_vertex_count() == 0,
               "activity tracker starts with no active vertices");

        expect(activity.active_edge_count(graph) == 0,
               "no active vertices means no active edges");

        activity.set_active(0);
        activity.set_active(2);

        expect(activity.is_active(0),
               "vertex 0 is active");

        expect(activity.is_active(2),
               "vertex 2 is active");

        expect(!activity.is_active(1),
               "vertex 1 remains inactive");

        expect(activity.active_vertex_count() == 2,
               "active vertex count");

        // Active edges = out_degree(0) + out_degree(2)
        //              = 2 + 1
        //              = 3.
        expect(activity.active_edge_count(graph) == 3,
               "active edge count");

        const auto vertices = activity.active_vertices();

        expect(vertices.size() == 2,
               "active vertex list size");

        expect(vertices[0] == 0,
               "active vertex list first vertex");

        expect(vertices[1] == 2,
               "active vertex list second vertex");
    }

    void test_activity_tracker_duplicate_activation()
    {
        using hytgraph::graph::ActivityTracker;
        using hytgraph::graph::CSRGraph;

        CSRGraph graph(
            3,
            {0, 1, 2, 2},
            {1, 2});

        ActivityTracker activity(graph.num_vertices());

        activity.set_active(1);
        activity.set_active(1);
        activity.set_active(1);

        expect(activity.active_vertex_count() == 1,
               "duplicate activation does not increase count");

        expect(activity.active_edge_count(graph) == 1,
               "duplicate activation does not duplicate edges");

        activity.set_active(1, false);

        expect(activity.active_vertex_count() == 0,
               "deactivation updates active count");

        expect(activity.active_edge_count(graph) == 0,
               "deactivation removes active edges");

        activity.set_active(1, false);

        expect(activity.active_vertex_count() == 0,
               "duplicate deactivation is harmless");
    }

    void test_activity_tracker_set_active_vertices()
    {
        using hytgraph::graph::ActivityTracker;
        using hytgraph::graph::CSRGraph;

        CSRGraph graph(
            4,
            {0, 2, 3, 4, 4},
            {1, 2, 2, 0});

        ActivityTracker activity(graph.num_vertices());

        activity.set_active_vertices({3, 0, 3});

        expect(activity.active_vertex_count() == 2,
               "set_active_vertices removes duplicate effect");

        expect(activity.is_active(0),
               "set_active_vertices activates vertex 0");

        expect(activity.is_active(3),
               "set_active_vertices activates vertex 3");

        expect(activity.active_edge_count(graph) == 2,
               "active edge count after replacing activity");

        activity.set_active_vertices({1});

        expect(activity.active_vertex_count() == 1,
               "set_active_vertices replaces previous activity");

        expect(!activity.is_active(0),
               "previously active vertex is cleared");

        expect(activity.is_active(1),
               "new active vertex is present");

        expect(activity.active_edge_count(graph) == 1,
               "active edge count after replacement");
    }

    void test_activity_tracker_clear()
    {
        using hytgraph::graph::ActivityTracker;
        using hytgraph::graph::CSRGraph;

        CSRGraph graph(
            3,
            {0, 1, 2, 2},
            {1, 2});

        ActivityTracker activity(graph.num_vertices());

        activity.set_active_vertices({0, 1});

        expect(activity.active_vertex_count() == 2,
               "activity populated before clear");

        activity.clear();

        expect(activity.active_vertex_count() == 0,
               "clear removes all active vertices");

        expect(activity.active_vertices().empty(),
               "clear produces empty active vertex list");

        expect(activity.active_edge_count(graph) == 0,
               "clear removes all active edges");
    }

    void test_activity_tracker_partition_statistics()
    {
        using hytgraph::graph::ActivityTracker;
        using hytgraph::graph::CSRGraph;

        // Graph:
        //
        // 0 -> 1, 2       degree 2
        // 1 -> 2         degree 1
        // 2 -> 0         degree 1
        // 3 -> nothing    degree 0
        //
        // Partition 0: vertices [0, 2)
        // Partition 1: vertices [2, 4)

        CSRGraph graph(
            4,
            {0, 2, 3, 4, 4},
            {1, 2, 2, 0});

        ActivityTracker activity(graph.num_vertices());

        // Active vertices are 0 and 2.
        activity.set_active_vertices({0, 2});

        const std::vector<ActivityTracker::VertexRange> partitions = {
            {0, 2},
            {2, 4}};

        const auto statistics =
            activity.partition_statistics(graph, partitions);

        expect(statistics.size() == 2,
               "partition statistics count");

        // Partition 0:
        // vertices 0 and 1
        // total edges = 2 + 1 = 3
        // active vertices = {0}
        // active edges = 2
        expect(statistics[0].total_edges == 3,
               "partition 0 total edges");

        expect(statistics[0].active_vertices == 1,
               "partition 0 active vertices");

        expect(statistics[0].active_edges == 2,
               "partition 0 active edges");

        expect(statistics[0].has_active_vertices(),
               "partition 0 reports active vertices");

        expect(statistics[0].has_active_edges(),
               "partition 0 reports active edges");

        // Partition 1:
        // vertices 2 and 3
        // total edges = 1 + 0 = 1
        // active vertices = {2}
        // active edges = 1
        expect(statistics[1].total_edges == 1,
               "partition 1 total edges");

        expect(statistics[1].active_vertices == 1,
               "partition 1 active vertices");

        expect(statistics[1].active_edges == 1,
               "partition 1 active edges");

        expect(statistics[1].has_active_vertices(),
               "partition 1 reports active vertices");

        expect(statistics[1].has_active_edges(),
               "partition 1 reports active edges");
    }

    void test_activity_tracker_empty_partition()
    {
        using hytgraph::graph::ActivityTracker;
        using hytgraph::graph::CSRGraph;

        CSRGraph graph(
            3,
            {0, 1, 2, 2},
            {1, 2});

        ActivityTracker activity(graph.num_vertices());

        activity.set_active(0);

        const std::vector<ActivityTracker::VertexRange> partitions = {
            {0, 1},
            {1, 3},
            {2, 2}};

        const auto statistics =
            activity.partition_statistics(graph, partitions);

        expect(statistics.size() == 3,
               "empty partition statistics count");

        expect(statistics[0].active_vertices == 1,
               "first partition active vertex count");

        expect(statistics[0].active_edges == 1,
               "first partition active edge count");

        expect(statistics[1].active_vertices == 0,
               "second partition has no active vertices");

        expect(statistics[1].active_edges == 0,
               "second partition has no active edges");

        expect(statistics[2].active_vertices == 0,
               "empty partition has no active vertices");

        expect(statistics[2].active_edges == 0,
               "empty partition has no active edges");

        expect(statistics[2].total_edges == 0,
               "empty partition has no total edges");
    }

    void test_activity_tracker_invalid_inputs()
    {
        using hytgraph::graph::ActivityTracker;
        using hytgraph::graph::CSRGraph;

        CSRGraph graph(
            3,
            {0, 1, 2, 2},
            {1, 2});

        ActivityTracker activity(graph.num_vertices());

        bool threw = false;

        try
        {
            activity.set_active(3);
        }
        catch (const std::out_of_range &)
        {
            threw = true;
        }

        expect(threw,
               "activity tracker rejects out-of-range vertex");

        threw = false;

        try
        {
            (void)activity.is_active(3);
        }
        catch (const std::out_of_range &)
        {
            threw = true;
        }

        expect(threw,
               "activity tracker rejects out-of-range query");

        threw = false;

        try
        {
            const std::vector<ActivityTracker::VertexRange> partitions = {
                {2, 1}};

            (void)activity.partition_statistics(graph, partitions);
        }
        catch (const std::invalid_argument &)
        {
            threw = true;
        }

        expect(threw,
               "activity tracker rejects reversed partition range");

        threw = false;

        try
        {
            const std::vector<ActivityTracker::VertexRange> partitions = {
                {0, 4}};

            (void)activity.partition_statistics(graph, partitions);
        }
        catch (const std::out_of_range &)
        {
            threw = true;
        }

        expect(threw,
               "activity tracker rejects partition range beyond graph");

        threw = false;

        try
        {
            CSRGraph different_graph(
                2,
                {0, 1, 1},
                {1});

            (void)activity.active_edge_count(different_graph);
        }
        catch (const std::invalid_argument &)
        {
            threw = true;
        }

        expect(threw,
               "activity tracker rejects mismatched graph");
    }

} // namespace

int main()
{
    try
    {
        test_pagerank_cycle();
        test_pagerank_dangling_vertex();
        test_pagerank_invalid_options();
        test_sssp_weighted();
        test_sssp_unweighted_and_unreachable();
        test_sssp_rejects_negative_weights();
        test_sssp_rejects_invalid_source();

        test_activity_tracker_basic();
        test_activity_tracker_duplicate_activation();
        test_activity_tracker_set_active_vertices();
        test_activity_tracker_clear();
        test_activity_tracker_partition_statistics();
        test_activity_tracker_empty_partition();
        test_activity_tracker_invalid_inputs();

        std::cout << "Phase 2 and Phase 3 CPU tests passed.\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}