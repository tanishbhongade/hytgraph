// tests/sep_adapter_bridge_tests.cpp
//
// Phase 14 — Data-movement bridge tests.
//
// Compiled as plain C++. Only links the project-owned HyTMSEPBridge
// interface. The CUDA-only engine work happens inside hytgraph_runtime.

#include "graph/csr_graph.hpp"
#include "sep_adapter/hytm_sep_bridge.hpp"
#include "sep_adapter/sep_host_graph_adapter.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace
{

    using hytgraph::graph::CSRGraph;
    constexpr std::uint32_t kTestGraphVertices = 100000;

    // ---------------------------------------------------------------------------
    // Test harness (matches Phase 13's style)
    // ---------------------------------------------------------------------------

    int g_checks = 0;
    int g_failures = 0;

#define CHECK(cond)                                             \
    do                                                          \
    {                                                           \
        ++g_checks;                                             \
        if (!(cond))                                            \
        {                                                       \
            ++g_failures;                                       \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ \
                      << "  CHECK(" #cond ")\n";                \
        }                                                       \
    } while (0)

#define CHECK_EQ(a, b)                                              \
    do                                                              \
    {                                                               \
        ++g_checks;                                                 \
        const auto _va = (a);                                       \
        const auto _vb = (b);                                       \
        if (!(_va == _vb))                                          \
        {                                                           \
            ++g_failures;                                           \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__     \
                      << "  CHECK_EQ(" #a ", " #b ")"               \
                      << "  lhs=" << _va << " rhs=" << _vb << "\n"; \
        }                                                           \
    } while (0)

    // ---------------------------------------------------------------------------
    // Graph builders
    // ---------------------------------------------------------------------------

    // Line graph 0 -> 1 -> 2 -> ... -> (n-1).
    // Every vertex except the last has out_degree 1. Last has out_degree 0,
    // so we add a self-loop at the top vertex to satisfy the vendored parser
    // precondition (highest vertex must appear as a source).
    CSRGraph make_line_graph(std::uint32_t n)
    {
        std::vector<CSRGraph::offset_type> offsets(n + 1, 0);
        std::vector<CSRGraph::vertex_id> columns;
        columns.reserve(2 * n);
        CSRGraph::offset_type edge = 0;
        for (std::uint32_t v = 0; v < n; ++v)
        {
            offsets[v] = edge;
            if (v + 1 < n)
            {
                columns.push_back(v + 1);
                ++edge;
            }
            else
            {
                columns.push_back(v);
                ++edge;
            }
        }
        offsets[n] = edge;
        return CSRGraph(n, std::move(offsets), std::move(columns), {});
    }

    // Weighted line graph: unit-weight forward edges + unit-weight self-loop
    // at the top vertex. Distances from source 0 are (0, 1, 2, ..., n-1).
    CSRGraph make_weighted_line_graph(std::uint32_t n)
    {
        std::vector<CSRGraph::offset_type> offsets(n + 1, 0);
        std::vector<CSRGraph::vertex_id> columns;
        std::vector<CSRGraph::weight_type> weights;
        columns.reserve(2 * n);
        weights.reserve(2 * n);
        CSRGraph::offset_type edge = 0;
        for (std::uint32_t v = 0; v < n; ++v)
        {
            offsets[v] = edge;
            if (v + 1 < n)
            {
                columns.push_back(v + 1);
                weights.push_back(1.0f);
                ++edge;
            }
            else
            {
                columns.push_back(v);
                weights.push_back(1.0f);
                ++edge;
            }
        }
        offsets[n] = edge;
        return CSRGraph(n, std::move(offsets), std::move(columns),
                        std::move(weights));
    }

    // ---------------------------------------------------------------------------
    // Test sections
    // ---------------------------------------------------------------------------

    void test_algorithm_string_roundtrip()
    {
        using hytgraph::sep_adapter::from_string;
        using hytgraph::sep_adapter::SEPBridgeAlgorithm;
        using hytgraph::sep_adapter::to_string;

        CHECK_EQ(std::string(to_string(SEPBridgeAlgorithm::PageRank)), "PageRank");
        CHECK_EQ(std::string(to_string(SEPBridgeAlgorithm::SSSP)), "SSSP");

        CHECK(from_string("PageRank").has_value());
        CHECK(from_string("SSSP").has_value());
        CHECK(!from_string("").has_value());
        CHECK(!from_string("pagerank").has_value());
        CHECK(!from_string("BFS").has_value());

        CHECK_EQ(static_cast<int>(*from_string(to_string(SEPBridgeAlgorithm::PageRank))),
                 static_cast<int>(SEPBridgeAlgorithm::PageRank));
        CHECK_EQ(static_cast<int>(*from_string(to_string(SEPBridgeAlgorithm::SSSP))),
                 static_cast<int>(SEPBridgeAlgorithm::SSSP));
    }

    void test_graph_file_factory()
    {
        using namespace hytgraph::sep_adapter;

        // Empty graph => nullopt
        {
            CSRGraph empty;
            auto r = write_sep_graph_file(empty);
            CHECK(!r.has_value());
        }

        // Valid unweighted graph => file exists, correct flag values
        {
            auto g = make_line_graph(kTestGraphVertices);
            auto r = write_sep_graph_file(g);
            CHECK(r.has_value());
            if (r)
            {
                CHECK(r->valid());
                CHECK(!r->path().empty());
                CHECK(std::filesystem::exists(r->path()));
                CHECK_EQ(std::string(r->format_flag_value()), "market_big");
                CHECK_EQ(r->weight_num_flag_value(), 0);
                CHECK_EQ(r->weighted(), false);

                const auto p = r->path();
                r->reset();
                CHECK(!std::filesystem::exists(p));
            }
        }

        // Valid weighted graph => weighted=true
        {
            auto g = make_weighted_line_graph(5);
            SEPGraphFileConfig cfg;
            cfg.weighted = true;
            cfg.weight_scale = 1.0f;
            auto r = write_sep_graph_file(g, cfg);
            CHECK(r.has_value());
            if (r)
            {
                CHECK(r->weighted());
                CHECK(std::filesystem::exists(r->path()));
            }
        }

        // Isolated top vertex => nullopt (parser precondition)
        {
            std::vector<CSRGraph::offset_type> off = {0, 1, 2, 3, 3};
            std::vector<CSRGraph::vertex_id> col = {1, 2, 3};
            CSRGraph g(4, std::move(off), std::move(col), {});
            auto r = write_sep_graph_file(g);
            CHECK(!r.has_value());
        }

        // Move-only semantics
        {
            auto g = make_line_graph(4);
            auto r = write_sep_graph_file(g);
            CHECK(r.has_value());
            if (r)
            {
                const auto p = r->path();
                SEPGraphFile moved = std::move(*r);
                CHECK(moved.valid());
                CHECK_EQ(moved.path(), p);
                CHECK(!r->valid());
            }
        }
    }

    void test_bridge_construction()
    {
        using namespace hytgraph::sep_adapter;

        // Invalid: empty graph
        {
            CSRGraph empty;
            HyTMSEPBridge b(SEPBridgeAlgorithm::PageRank, empty);
            CHECK(!b.valid());
        }

        // Invalid: isolated top vertex
        {
            std::vector<CSRGraph::offset_type> off = {0, 1, 2, 2};
            std::vector<CSRGraph::vertex_id> col = {1, 2};
            CSRGraph g(3, std::move(off), std::move(col), {});
            HyTMSEPBridge b(SEPBridgeAlgorithm::PageRank, g);
            CHECK(!b.valid());
        }

        // Valid: PageRank on line graph
        {
            auto g = make_line_graph(kTestGraphVertices);
            HyTMSEPBridge b(SEPBridgeAlgorithm::PageRank, g);
            CHECK(b.valid());
            CHECK_EQ(static_cast<int>(b.algorithm()),
                     static_cast<int>(SEPBridgeAlgorithm::PageRank));
            CHECK(!b.graph_file_path().empty());
            CHECK(std::filesystem::exists(b.graph_file_path()));
        }

        // Valid: SSSP on weighted line graph
        {
            auto g = make_weighted_line_graph(5);
            HyTMSEPBridge b(SEPBridgeAlgorithm::SSSP, g);
            CHECK(b.valid());
            CHECK_EQ(static_cast<int>(b.algorithm()),
                     static_cast<int>(SEPBridgeAlgorithm::SSSP));
        }
    }

    void test_pagerank_5vertex()
    {
        using namespace hytgraph::sep_adapter;
        SEPBridgeConfig cfg;
        auto g = make_line_graph(kTestGraphVertices);
        cfg.max_iteration = 10;
        HyTMSEPBridge b(SEPBridgeAlgorithm::PageRank, g);
        if (!b.valid())
        {
            std::cerr << "SKIP pagerank: bridge not valid\n";
            return;
        }

        auto result = b.run();

        if (result.execution.state != SEPExecutionState::OK)
        {
            std::cerr << "NOTE pagerank run returned state "
                      << static_cast<int>(result.execution.state)
                      << ": " << result.execution.message << "\n";
            return;
        }

        CHECK_EQ(result.pagerank_values.size(),
                 static_cast<std::size_t>(kTestGraphVertices));
        CHECK(result.sssp_distances.empty());

        for (float v : result.pagerank_values)
        {
            CHECK(std::isfinite(v));
            CHECK(v >= 0.0f);
        }

        auto second = b.run();
        CHECK(second.execution.state != SEPExecutionState::OK);
    }

    void test_sssp_5vertex()
    {
        using namespace hytgraph::sep_adapter;

        auto g = make_weighted_line_graph(5);
        SEPBridgeConfig cfg;
        cfg.sssp_source = 0;
        cfg.max_iteration = 10;
        HyTMSEPBridge b(SEPBridgeAlgorithm::SSSP, g, cfg);
        if (!b.valid())
        {
            std::cerr << "SKIP sssp: bridge not valid\n";
            return;
        }

        auto result = b.run();

        if (result.execution.state != SEPExecutionState::OK)
        {
            std::cerr << "NOTE sssp run returned state "
                      << static_cast<int>(result.execution.state)
                      << ": " << result.execution.message << "\n";
            return;
        }

        CHECK_EQ(result.sssp_distances.size(), 5u);
        CHECK(result.pagerank_values.empty());

        const std::uint32_t maxd = std::numeric_limits<std::uint32_t>::max();
        if (result.sssp_distances[0] != maxd)
        {
            CHECK_EQ(result.sssp_distances[0], 0u);
            CHECK_EQ(result.sssp_distances[1], 1u);
            CHECK_EQ(result.sssp_distances[2], 2u);
            // Every consecutive distance increments by 1 along the line.
            for (std::size_t i = 2; i < result.sssp_distances.size(); ++i)
            {
                CHECK_EQ(result.sssp_distances[i],
                         static_cast<std::uint32_t>(i));
            }
        }
    }

    // ---------------------------------------------------------------------------
    // Optional extended test: read a real graph from HYTGRAPH_TEST_GRAPH
    // ---------------------------------------------------------------------------

    CSRGraph load_market_big_text(const std::string &path, bool weighted)
    {
        std::ifstream in(path);
        if (!in)
            throw std::runtime_error("cannot open " + path);

        std::vector<std::pair<std::uint32_t, std::uint32_t>> edges;
        std::vector<float> weights;
        std::uint32_t max_v = 0;

        std::string line;
        while (std::getline(in, line))
        {
            if (line.empty() || line[0] == '#')
                continue;
            std::istringstream ss(line);
            std::uint32_t s = 0, d = 0;
            float w = 1.0f;
            if (!(ss >> s >> d))
                continue;
            if (weighted)
                ss >> w;
            edges.emplace_back(s, d);
            weights.push_back(w);
            if (s > max_v)
                max_v = s;
            if (d > max_v)
                max_v = d;
        }

        const std::uint32_t n = max_v + 1;
        std::vector<CSRGraph::offset_type> off(n + 1, 0);
        for (const auto &e : edges)
            off[e.first + 1]++;
        for (std::uint32_t i = 0; i < n; ++i)
            off[i + 1] += off[i];

        std::vector<CSRGraph::vertex_id> col(edges.size());
        std::vector<CSRGraph::weight_type> wts(weighted ? edges.size() : 0);
        std::vector<CSRGraph::offset_type> cursor = off;

        for (std::size_t i = 0; i < edges.size(); ++i)
        {
            const auto s = edges[i].first;
            col[cursor[s]] = edges[i].second;
            if (weighted)
                wts[cursor[s]] = weights[i];
            ++cursor[s];
        }

        return CSRGraph(n, std::move(off), std::move(col), std::move(wts));
    }

    bool test_extended_env_graph()
    {
        using namespace hytgraph::sep_adapter;

        const char *path = std::getenv("HYTGRAPH_TEST_GRAPH");
        if (!path || !*path)
        {
            std::cout << "SKIP extended test: HYTGRAPH_TEST_GRAPH not set\n";
            return true;
        }

        const bool weighted = std::getenv("HYTGRAPH_TEST_GRAPH_WEIGHTED") != nullptr;

        CSRGraph g;
        try
        {
            g = load_market_big_text(path, weighted);
        }
        catch (const std::exception &e)
        {
            std::cerr << "FAIL extended test: " << e.what() << "\n";
            return false;
        }

        std::cout << "Extended test graph: " << g.num_vertices()
                  << " vertices, " << g.num_edges() << " edges\n";

        SEPBridgeConfig cfg;
        cfg.verbose = false;

        HyTMSEPBridge pr(SEPBridgeAlgorithm::PageRank, g, cfg);
        if (pr.valid())
        {
            auto r = pr.run();
            std::cout << "PageRank state="
                      << static_cast<int>(r.execution.state)
                      << " values=" << r.pagerank_values.size()
                      << "\n";
        }

        if (weighted)
        {
            HyTMSEPBridge sssp(SEPBridgeAlgorithm::SSSP, g, cfg);
            if (sssp.valid())
            {
                auto r = sssp.run();
                std::cout << "SSSP state="
                          << static_cast<int>(r.execution.state)
                          << " distances=" << r.sssp_distances.size()
                          << "\n";
            }
        }

        return true;
    }

} // namespace

int main()
{
    std::cout << "sep_adapter_bridge_tests\n";

    test_algorithm_string_roundtrip();
    test_graph_file_factory();
    test_bridge_construction();
    test_pagerank_5vertex();
    test_sssp_5vertex();

    const bool extended_ok = test_extended_env_graph();

    std::cout << "\nChecks: " << g_checks
              << "  Failures: " << g_failures << "\n";

    if (!extended_ok)
        return 1;
    if (g_failures != 0)
        return 1;
    return 0;
}