#include "algorithms/pagerank.hpp"
#include "algorithms/sssp.hpp"

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

        std::cout << "Phase 2 CPU algorithm tests passed.\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}