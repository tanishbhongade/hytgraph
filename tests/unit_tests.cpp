#include "runtime/config.hpp"
#include "runtime/result.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include "graph/csr_graph.hpp"
#include <stdexcept>
#include "graph/graph_loader.hpp"

#include "graph/partition.hpp"

namespace
{

    void expect(bool condition, const std::string &message)
    {
        if (!condition)
            throw std::runtime_error("FAIL: " + message);
    }

    void test_config()
    {
        using hytgraph::runtime::Config;
        Config config;
        config.set("algorithm", "pagerank");
        config.set("partition.size_bytes", "33554432");
        config.set("cache.enabled", "true");
        config.set("transfer.gamma", "0.625");

        expect(config.contains("algorithm"), "config contains inserted key");
        expect(config.get("algorithm") == "pagerank", "string lookup");
        expect(config.get_int("partition.size_bytes") == 33554432, "integer conversion");
        expect(config.get_bool("cache.enabled"), "boolean conversion");
        expect(config.get_double("transfer.gamma") == 0.625, "double conversion");
        expect(config.get_or("missing", "fallback") == "fallback", "fallback lookup");
    }

    void test_result_json()
    {
        using hytgraph::runtime::Result;
        using hytgraph::runtime::result_to_json;

        Result result;
        result.experiment_id = "unit-test";
        result.timestamp_utc = "2026-01-01T00:00:00Z";
        result.config["algorithm"] = "pagerank";
        result.measurements["runtime_seconds"] = 1.25;

        const std::string json = result_to_json(result);
        expect(json.find("\"schema_version\": \"0.1\"") != std::string::npos,
               "schema version emitted");
        expect(json.find("\"experiment_id\": \"unit-test\"") != std::string::npos,
               "experiment id emitted");
        expect(json.find("\"runtime_seconds\": 1.25") != std::string::npos,
               "measurement emitted");
    }

    void test_result_file()
    {
        using hytgraph::runtime::Result;
        using hytgraph::runtime::write_result_json;

        const auto path = std::filesystem::temp_directory_path() / "hytgraph_phase0_test.json";
        Result result;
        result.experiment_id = "file-test";
        result.timestamp_utc = "2026-01-01T00:00:00Z";

        expect(write_result_json(result, path.string()), "result file writes successfully");
        std::ifstream file(path);
        expect(file.good(), "result file exists");
        std::filesystem::remove(path);
    }

    void test_csr_basic_queries()
    {
        using hytgraph::graph::CSRGraph;

        // Graph:
        //
        // 0 -> 1, 2
        // 1 -> 2
        // 2 -> 0
        // 3 -> nothing
        //
        // CSR:
        // row_offsets     = [0, 2, 3, 4, 4]
        // column_indices  = [1, 2, 2, 0]

        CSRGraph graph(
            4,
            {0, 2, 3, 4, 4},
            {1, 2, 2, 0});

        expect(graph.num_vertices() == 4, "CSR vertex count");
        expect(graph.num_edges() == 4, "CSR edge count");
        expect(!graph.has_weights(), "unweighted CSR");

        expect(graph.out_degree(0) == 2, "vertex 0 degree");
        expect(graph.out_degree(1) == 1, "vertex 1 degree");
        expect(graph.out_degree(2) == 1, "vertex 2 degree");
        expect(graph.out_degree(3) == 0, "isolated vertex degree");

        const auto [begin, end] = graph.neighbor_range(0);

        expect(begin == 0, "vertex 0 neighbor range begin");
        expect(end == 2, "vertex 0 neighbor range end");

        expect(graph.neighbor_at(0) == 1, "first edge destination");
        expect(graph.neighbor_at(1) == 2, "second edge destination");
        expect(graph.neighbor_at(2) == 2, "third edge destination");
        expect(graph.neighbor_at(3) == 0, "fourth edge destination");
    }

    void test_csr_weighted_graph()
    {
        using hytgraph::graph::CSRGraph;

        CSRGraph graph(
            3,
            {0, 2, 3, 3},
            {1, 2, 2},
            {0.5F, 1.0F, 2.5F});

        expect(graph.has_weights(), "weighted CSR");

        expect(graph.weight_at(0) == 0.5F, "first edge weight");
        expect(graph.weight_at(1) == 1.0F, "second edge weight");
        expect(graph.weight_at(2) == 2.5F, "third edge weight");
    }

    void test_csr_invalid_vertex_queries()
    {
        using hytgraph::graph::CSRGraph;

        CSRGraph graph(
            2,
            {0, 1, 1},
            {1});

        bool threw = false;

        try
        {
            (void)graph.out_degree(2);
        }
        catch (const std::out_of_range &)
        {
            threw = true;
        }

        expect(threw, "out-of-range vertex query is rejected");

        threw = false;

        try
        {
            (void)graph.neighbor_at(1);
        }
        catch (const std::out_of_range &)
        {
            threw = true;
        }

        expect(threw, "out-of-range edge query is rejected");
    }

    void test_csr_invalid_representations()
    {
        using hytgraph::graph::CSRGraph;

        bool threw = false;

        try
        {
            CSRGraph graph(
                2,
                {0, 1},
                {1});
            (void)graph;
        }
        catch (const std::invalid_argument &)
        {
            threw = true;
        }

        expect(threw, "incorrect row offset size is rejected");

        threw = false;

        try
        {
            CSRGraph graph(
                2,
                {0, 2, 1},
                {1, 0});
            (void)graph;
        }
        catch (const std::invalid_argument &)
        {
            threw = true;
        }

        expect(threw, "decreasing row offsets are rejected");

        threw = false;

        try
        {
            CSRGraph graph(
                2,
                {0, 1, 2},
                {1, 2});
            (void)graph;
        }
        catch (const std::invalid_argument &)
        {
            threw = true;
        }

        expect(threw, "invalid destination vertex is rejected");

        threw = false;

        try
        {
            CSRGraph graph(
                2,
                {0, 1, 1},
                {1},
                {1.0F, 2.0F});
            (void)graph;
        }
        catch (const std::invalid_argument &)
        {
            threw = true;
        }

        expect(threw, "incorrect weight count is rejected");
    }

    void test_graph_loader_unweighted()
    {
        using hytgraph::graph::GraphLoader;

        const auto path =
            std::filesystem::temp_directory_path() /
            "hytgraph_phase1_unweighted.edgelist";

        {
            std::ofstream file(path);
            expect(file.good(), "temporary unweighted graph file opens");

            file << "# simple directed graph\n";
            file << "0 1\n";
            file << "0 2\n";
            file << "1 2\n";
            file << "2 0\n";
        }

        const auto graph =
            GraphLoader::load_edge_list(path.string(), 4, false);

        expect(graph.num_vertices() == 4,
               "loader preserves vertex count");

        expect(graph.num_edges() == 4,
               "loader preserves edge count");

        expect(!graph.has_weights(),
               "unweighted loader produces unweighted graph");

        expect(graph.out_degree(0) == 2,
               "loaded vertex 0 degree");

        expect(graph.out_degree(1) == 1,
               "loaded vertex 1 degree");

        expect(graph.out_degree(2) == 1,
               "loaded vertex 2 degree");

        expect(graph.out_degree(3) == 0,
               "loaded isolated vertex degree");

        expect(graph.neighbor_at(0) == 1,
               "loaded first neighbor");

        expect(graph.neighbor_at(1) == 2,
               "loaded second neighbor");

        expect(graph.neighbor_at(2) == 2,
               "loaded third neighbor");

        expect(graph.neighbor_at(3) == 0,
               "loaded fourth neighbor");

        std::filesystem::remove(path);
    }

    void test_graph_loader_weighted()
    {
        using hytgraph::graph::GraphLoader;

        const auto path =
            std::filesystem::temp_directory_path() /
            "hytgraph_phase1_weighted.edgelist";

        {
            std::ofstream file(path);
            expect(file.good(), "temporary weighted graph file opens");

            file << "# weighted graph\n";
            file << "0 1 0.5\n";
            file << "0 2 1.25\n";
            file << "1 2 2.5\n";
        }

        const auto graph =
            GraphLoader::load_edge_list(path.string(), 3, true);

        expect(graph.num_vertices() == 3,
               "weighted loader preserves vertex count");

        expect(graph.num_edges() == 3,
               "weighted loader preserves edge count");

        expect(graph.has_weights(),
               "weighted loader produces weighted graph");

        expect(graph.weight_at(0) == 0.5F,
               "first loaded edge weight");

        expect(graph.weight_at(1) == 1.25F,
               "second loaded edge weight");

        expect(graph.weight_at(2) == 2.5F,
               "third loaded edge weight");

        std::filesystem::remove(path);
    }

    void test_graph_loader_parallel_edges()
    {
        using hytgraph::graph::GraphLoader;

        const auto path =
            std::filesystem::temp_directory_path() /
            "hytgraph_phase1_parallel.edgelist";

        {
            std::ofstream file(path);
            expect(file.good(), "temporary parallel-edge graph file opens");

            file << "0 1\n";
            file << "0 1\n";
            file << "0 2\n";
        }

        const auto graph =
            GraphLoader::load_edge_list(path.string(), 3, false);

        expect(graph.num_edges() == 3,
               "loader preserves parallel edges");

        expect(graph.out_degree(0) == 3,
               "parallel edges contribute to degree");

        expect(graph.neighbor_at(0) == 1,
               "first parallel edge preserved");

        expect(graph.neighbor_at(1) == 1,
               "second parallel edge preserved");

        expect(graph.neighbor_at(2) == 2,
               "third edge preserved");

        std::filesystem::remove(path);
    }

    void test_graph_loader_invalid_input()
    {
        using hytgraph::graph::GraphLoader;

        {
            const auto path =
                std::filesystem::temp_directory_path() /
                "hytgraph_phase1_invalid_vertex.edgelist";

            {
                std::ofstream file(path);
                expect(file.good(), "invalid-vertex test file opens");
                file << "0 3\n";
            }

            bool threw = false;

            try
            {
                (void)GraphLoader::load_edge_list(path.string(), 3, false);
            }
            catch (const std::invalid_argument &)
            {
                threw = true;
            }

            expect(threw,
                   "loader rejects destination vertex outside range");

            std::filesystem::remove(path);
        }

        {
            const auto path =
                std::filesystem::temp_directory_path() /
                "hytgraph_phase1_missing_weight.edgelist";

            {
                std::ofstream file(path);
                expect(file.good(), "missing-weight test file opens");
                file << "0 1\n";
            }

            bool threw = false;

            try
            {
                (void)GraphLoader::load_edge_list(path.string(), 2, true);
            }
            catch (const std::invalid_argument &)
            {
                threw = true;
            }

            expect(threw,
                   "weighted loader rejects missing edge weight");

            std::filesystem::remove(path);
        }

        {
            const auto path =
                std::filesystem::temp_directory_path() /
                "hytgraph_phase1_malformed.edgelist";

            {
                std::ofstream file(path);
                expect(file.good(), "malformed test file opens");
                file << "this is not an edge\n";
            }

            bool threw = false;

            try
            {
                (void)GraphLoader::load_edge_list(path.string(), 2, false);
            }
            catch (const std::invalid_argument &)
            {
                threw = true;
            }

            expect(threw,
                   "loader rejects malformed edge-list records");

            std::filesystem::remove(path);
        }
    }

    void test_logical_partition_basic()
    {
        using hytgraph::graph::CSRGraph;
        using hytgraph::graph::LogicalPartitioner;

        // Graph:
        //
        // 0 -> 1, 2
        // 1 -> 2
        // 2 -> 0
        // 3 -> 0
        //
        // CSR:
        // row_offsets    = [0, 2, 3, 4, 5]
        // column_indices = [1, 2, 2, 0, 0]
        //
        // Use a very small target so that multiple logical partitions
        // are created for this test.

        CSRGraph graph(
            4,
            {0, 2, 3, 4, 5},
            {1, 2, 2, 0, 0});

        LogicalPartitioner partitioner(
            2U * sizeof(CSRGraph::vertex_id));

        const auto partitions = partitioner.partition(graph);

        expect(!partitions.empty(),
               "partitioner creates at least one partition");

        expect(partitions.front().vertex_begin() == 0,
               "first partition begins at vertex zero");

        expect(partitions.back().vertex_end() == graph.num_vertices(),
               "last partition ends at num_vertices");

        CSRGraph::offset_type total_edges = 0;

        CSRGraph::vertex_id expected_vertex_begin = 0;
        CSRGraph::offset_type expected_edge_begin = 0;

        for (const auto &partition : partitions)
        {
            expect(partition.vertex_begin() == expected_vertex_begin,
                   "partitions have contiguous vertex ranges");

            expect(partition.edge_begin() == expected_edge_begin,
                   "partitions have contiguous edge ranges");

            expect(partition.vertex_begin() <= partition.vertex_end(),
                   "partition vertex range is valid");

            expect(partition.edge_begin() <= partition.edge_end(),
                   "partition edge range is valid");

            total_edges += partition.edge_count();

            expected_vertex_begin = partition.vertex_end();
            expected_edge_begin = partition.edge_end();
        }

        expect(total_edges == graph.num_edges(),
               "partition edge totals equal graph edge count");
    }

    void test_logical_partition_default_size()
    {
        using hytgraph::graph::LogicalPartitioner;

        LogicalPartitioner partitioner;

        expect(
            partitioner.partition_bytes() ==
                LogicalPartitioner::kDefaultPartitionBytes,
            "default logical partition size is 32 MiB");
    }

    void test_logical_partition_configuration()
    {
        using hytgraph::graph::LogicalPartitioner;

        constexpr std::size_t custom_size = 1024U;

        LogicalPartitioner partitioner(custom_size);

        expect(partitioner.partition_bytes() == custom_size,
               "custom logical partition size is preserved");
    }

    void test_logical_partition_empty_graph()
    {
        using hytgraph::graph::CSRGraph;
        using hytgraph::graph::LogicalPartitioner;

        CSRGraph graph(
            0,
            {0},
            {});

        LogicalPartitioner partitioner;

        const auto partitions = partitioner.partition(graph);

        expect(partitions.empty(),
               "empty graph produces no logical partitions");
    }

    void test_logical_partition_zero_degree_vertices()
    {
        using hytgraph::graph::CSRGraph;
        using hytgraph::graph::LogicalPartitioner;

        // Four isolated vertices.
        CSRGraph graph(
            4,
            {0, 0, 0, 0, 0},
            {});

        LogicalPartitioner partitioner(
            sizeof(CSRGraph::vertex_id));

        const auto partitions = partitioner.partition(graph);

        expect(!partitions.empty(),
               "graph with isolated vertices still produces partitions");

        expect(partitions.front().vertex_begin() == 0,
               "isolated-vertex graph starts at vertex zero");

        expect(partitions.back().vertex_end() == graph.num_vertices(),
               "isolated-vertex graph covers all vertices");

        CSRGraph::offset_type total_edges = 0;

        for (const auto &partition : partitions)
        {
            total_edges += partition.edge_count();
        }

        expect(total_edges == 0,
               "isolated-vertex graph has zero partition edges");
    }

    void test_logical_partition_large_vertex()
    {
        using hytgraph::graph::CSRGraph;
        using hytgraph::graph::LogicalPartitioner;

        // Vertex 0 has four outgoing edges. The requested partition
        // size is smaller than this adjacency list, so the vertex cannot
        // be split across logical partitions.
        CSRGraph graph(
            3,
            {0, 4, 4, 4},
            {1, 2, 1, 2});

        LogicalPartitioner partitioner(
            2U * sizeof(CSRGraph::vertex_id));

        const auto partitions = partitioner.partition(graph);

        expect(!partitions.empty(),
               "large adjacency vertex still produces a partition");

        expect(partitions.front().vertex_begin() == 0,
               "large adjacency partition begins at vertex zero");

        expect(partitions.front().vertex_end() >= 1,
               "large adjacency vertex remains intact");

        CSRGraph::offset_type total_edges = 0;

        for (const auto &partition : partitions)
        {
            total_edges += partition.edge_count();
        }

        expect(total_edges == graph.num_edges(),
               "large adjacency vertex does not lose edges");
    }

    void test_logical_partition_invalid_size()
    {
        using hytgraph::graph::LogicalPartitioner;

        bool threw = false;

        try
        {
            LogicalPartitioner partitioner(0);
            (void)partitioner;
        }
        catch (const std::invalid_argument &)
        {
            threw = true;
        }

        expect(threw,
               "zero partition size is rejected");
    }
    void test_logical_partition_edge_boundaries()
    {
        using hytgraph::graph::CSRGraph;
        using hytgraph::graph::LogicalPartitioner;

        CSRGraph graph(
            5,
            {0, 2, 2, 5, 6, 8},
            {1, 2, 0, 1, 4, 3, 0, 2});

        LogicalPartitioner partitioner(
            2U * sizeof(CSRGraph::vertex_id));

        const auto partitions = partitioner.partition(graph);

        expect(!partitions.empty(),
               "edge-boundary test creates partitions");

        for (std::size_t i = 0; i < partitions.size(); ++i)
        {
            const auto &partition = partitions[i];

            expect(
                partition.edge_begin() ==
                    graph.neighbor_range(partition.vertex_begin()).first,
                "partition edge_begin matches CSR row boundary");

            expect(
                partition.edge_end() ==
                    graph.neighbor_range(partition.vertex_end() - 1U).second,
                "partition edge_end matches CSR row boundary");

            if (i > 0)
            {
                const auto &previous = partitions[i - 1];

                expect(
                    previous.vertex_end() == partition.vertex_begin(),
                    "adjacent partitions share exactly one vertex boundary");

                expect(
                    previous.edge_end() == partition.edge_begin(),
                    "adjacent partitions share exactly one edge boundary");
            }
        }
    }

    void test_logical_partition_full_vertex_coverage()
    {
        using hytgraph::graph::CSRGraph;
        using hytgraph::graph::LogicalPartitioner;

        CSRGraph graph(
            8,
            {0, 1, 3, 3, 6, 7, 9, 9, 10},
            {1, 2, 3, 0, 4, 5, 6, 2, 7, 1});

        LogicalPartitioner partitioner(
            2U * sizeof(CSRGraph::vertex_id));

        const auto partitions = partitioner.partition(graph);

        expect(!partitions.empty(),
               "coverage test creates partitions");

        CSRGraph::vertex_id next_vertex = 0;

        for (const auto &partition : partitions)
        {
            expect(
                partition.vertex_begin() == next_vertex,
                "partition sequence has no vertex gaps or overlaps");

            next_vertex = partition.vertex_end();
        }

        expect(
            next_vertex == graph.num_vertices(),
            "partitions cover every graph vertex");
    }

    void test_logical_partition_full_edge_coverage()
    {
        using hytgraph::graph::CSRGraph;
        using hytgraph::graph::LogicalPartitioner;

        CSRGraph graph(
            6,
            {0, 3, 3, 5, 8, 8, 10},
            {1, 2, 3, 0, 4, 0, 1, 5, 2, 3});

        LogicalPartitioner partitioner(
            2U * sizeof(CSRGraph::vertex_id));

        const auto partitions = partitioner.partition(graph);

        expect(!partitions.empty(),
               "edge coverage test creates partitions");

        CSRGraph::offset_type next_edge = 0;

        for (const auto &partition : partitions)
        {
            expect(
                partition.edge_begin() == next_edge,
                "partition sequence has no edge gaps or overlaps");

            next_edge = partition.edge_end();
        }

        expect(
            next_edge == graph.num_edges(),
            "partitions cover every graph edge");
    }
} // namespace

int main()
{
    try
    {
        test_config();
        test_result_json();
        test_result_file();

        test_csr_basic_queries();
        test_csr_weighted_graph();
        test_csr_invalid_vertex_queries();
        test_csr_invalid_representations();

        test_graph_loader_unweighted();
        test_graph_loader_weighted();
        test_graph_loader_parallel_edges();
        test_graph_loader_invalid_input();

        test_logical_partition_edge_boundaries();
        test_logical_partition_full_vertex_coverage();
        test_logical_partition_full_edge_coverage();

        std::cout << "All Phase 0 unit tests passed.\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
