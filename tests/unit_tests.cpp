#include "runtime/config.hpp"
#include "runtime/result.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <cmath>

#include "graph/csr_graph.hpp"
#include <stdexcept>
#include "graph/graph_loader.hpp"

#include "graph/partition.hpp"
#include "transfer/filter_engine.hpp"

#include "transfer/compaction_engine.hpp"
#include "transfer/zero_copy_engine.hpp"
#include "transfer/hytm_cost_model.hpp"

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

    void test_exp_tm_filter_skips_inactive_partitions()
    {
        using hytgraph::graph::ActivityTracker;
        using hytgraph::graph::CSRGraph;
        using hytgraph::graph::LogicalPartitioner;
        using hytgraph::transfer::ExpTMFilter;

        // Graph:
        //
        // 0 -> 1, 2
        // 1 -> 2
        // 2 -> 0
        // 3 -> 0
        //
        // Five total edges. Use a small partition target so that
        // the graph is divided into multiple logical partitions.

        CSRGraph graph(
            4,
            {0, 2, 3, 4, 5},
            {1, 2, 2, 0, 0});

        LogicalPartitioner partitioner(
            2U * sizeof(CSRGraph::vertex_id));

        const auto partitions = partitioner.partition(graph);

        ActivityTracker activity(graph.num_vertices());

        // Only vertex 0 is active. Its two outgoing edges therefore
        // make the partition containing vertex 0 active.
        activity.set_active(0);

        ExpTMFilter filter(true);

        const auto plan =
            filter.plan(graph, partitions, activity);

        expect(plan.partitions.size() == partitions.size(),
               "filter plan contains every logical partition");

        expect(plan.active_partition_count() >= 1U,
               "active vertex produces an active partition");

        expect(plan.transferred_partition_count() ==
                   plan.active_partition_count(),
               "filter transfers exactly the active partitions");

        expect(plan.transferred_edge_count() > 0U,
               "filter transfers edges from active partitions");

        expect(plan.transferred_edge_count() < graph.num_edges(),
               "filter skips at least one inactive partition");

        std::size_t expected_transferred_bytes = 0U;
        CSRGraph::offset_type expected_transferred_edges = 0U;

        for (std::size_t i = 0U; i < partitions.size(); ++i)
        {
            if (!plan.partitions[i].transferred)
            {
                continue;
            }

            expected_transferred_edges +=
                partitions[i].edge_count();

            expected_transferred_bytes +=
                partitions[i].edge_data_bytes();
        }

        expect(
            plan.transferred_edge_count() ==
                expected_transferred_edges,
            "reported transferred edge count matches selected partitions");

        expect(
            plan.transferred_byte_count() ==
                expected_transferred_bytes,
            "reported transferred byte count matches selected partitions");

        for (std::size_t i = 0U; i < partitions.size(); ++i)
        {
            const auto &decision = plan.partitions[i];

            expect(
                decision.total_edges == partitions[i].edge_count(),
                "filter decision records partition edge count");

            if (decision.active)
            {
                expect(
                    decision.transferred_edges ==
                        partitions[i].edge_count(),
                    "active partition is transferred in full");

                expect(
                    decision.transferred_bytes ==
                        partitions[i].edge_data_bytes(),
                    "active partition transfers complete edge payload");
            }
            else
            {
                expect(
                    decision.transferred_edges == 0U,
                    "inactive partition transfers no edges");

                expect(
                    decision.transferred_bytes == 0U,
                    "inactive partition transfers no bytes");
            }
        }
    }

    void test_exp_tm_filter_full_partition_transfer()
    {
        using hytgraph::graph::ActivityTracker;
        using hytgraph::graph::CSRGraph;
        using hytgraph::graph::LogicalPartitioner;
        using hytgraph::transfer::ExpTMFilter;

        // The first partition contains two edges. Activate only one
        // vertex in that partition. ExpTM-Filter must still transfer
        // both edges because it does not compact the partition.

        CSRGraph graph(
            3,
            {0, 2, 2, 2},
            {1, 2});

        LogicalPartitioner partitioner(
            2U * sizeof(CSRGraph::vertex_id));

        const auto partitions = partitioner.partition(graph);

        expect(!partitions.empty(),
               "full-transfer test creates logical partitions");

        ActivityTracker activity(graph.num_vertices());
        activity.set_active(0);

        ExpTMFilter filter(true);

        const auto plan =
            filter.plan(graph, partitions, activity);

        expect(plan.active_partition_count() == 1U,
               "only partition containing active edges is active");

        expect(plan.transferred_partition_count() == 1U,
               "exactly one partition is transferred");

        expect(plan.transferred_edge_count() ==
                   partitions.front().edge_count(),
               "filter transfers entire active partition");

        expect(plan.transferred_edge_count() == 2U,
               "filter does not compact active edges");
    }

    void test_exp_tm_filter_disabled_reference_path()
    {
        using hytgraph::graph::ActivityTracker;
        using hytgraph::graph::CSRGraph;
        using hytgraph::graph::LogicalPartitioner;
        using hytgraph::transfer::ExpTMFilter;

        CSRGraph graph(
            4,
            {0, 1, 2, 3, 4},
            {1, 2, 3, 0});

        LogicalPartitioner partitioner(
            2U * sizeof(CSRGraph::vertex_id));

        const auto partitions = partitioner.partition(graph);

        ActivityTracker activity(graph.num_vertices());

        // No active vertices.
        expect(activity.active_vertex_count() == 0U,
               "reference-path test starts with no active vertices");

        ExpTMFilter filter(false);

        const auto plan =
            filter.plan(graph, partitions, activity);

        expect(plan.active_partition_count() == 0U,
               "no partitions are active");

        expect(plan.transferred_partition_count() ==
                   partitions.size(),
               "disabled filter transfers every partition");

        expect(plan.transferred_edge_count() ==
                   graph.num_edges(),
               "disabled filter transfers all graph edges");

        expect(plan.transferred_byte_count() ==
                   graph.num_edges() *
                       sizeof(CSRGraph::vertex_id),
               "disabled filter transfers complete edge payload");
    }

    void test_exp_tm_filter_rejects_mismatched_activity()
    {
        using hytgraph::graph::ActivityTracker;
        using hytgraph::graph::CSRGraph;
        using hytgraph::graph::LogicalPartitioner;
        using hytgraph::transfer::ExpTMFilter;

        CSRGraph graph(
            3,
            {0, 1, 2, 2},
            {1, 2});

        LogicalPartitioner partitioner(
            2U * sizeof(CSRGraph::vertex_id));

        const auto partitions = partitioner.partition(graph);

        ActivityTracker activity(2);

        ExpTMFilter filter(true);

        bool threw = false;

        try
        {
            (void)filter.plan(graph, partitions, activity);
        }
        catch (const std::invalid_argument &)
        {
            threw = true;
        }

        expect(threw,
               "filter rejects activity tracker with wrong vertex count");
    }

    void test_exp_tm_compaction_basic()
    {
        using hytgraph::graph::ActivityTracker;
        using hytgraph::graph::CSRGraph;
        using hytgraph::graph::LogicalPartitioner;
        using hytgraph::transfer::ExpTMCompaction;

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

        CSRGraph graph(
            4,
            {0, 2, 3, 4, 5},
            {1, 2, 2, 0, 0});

        LogicalPartitioner partitioner(
            2U * sizeof(CSRGraph::vertex_id));

        const auto partitions = partitioner.partition(graph);

        ActivityTracker activity(graph.num_vertices());

        // Only vertex 0 is active.
        activity.set_active(0);

        ExpTMCompaction compaction(true);

        const auto result =
            compaction.compact(graph, partitions, activity);

        expect(compaction.enabled(),
               "ExpTM-Compaction is enabled");

        expect(
            result.compacted_partition_count() == 1U,
            "only partition containing the active vertex is compacted");

        expect(
            result.active_vertex_count() == 1U,
            "one active source vertex is represented");

        expect(
            result.active_edge_count() == 2U,
            "active source contributes exactly two edges");

        expect(
            result.neighbor_bytes() ==
                2U * sizeof(CSRGraph::vertex_id),
            "neighbor payload contains two destination IDs");

        expect(
            result.index_bytes() ==
                2U * sizeof(CSRGraph::offset_type),
            "compressed index contains one range per active vertex plus terminal offset");

        expect(
            result.total_bytes() ==
                result.neighbor_bytes() + result.index_bytes(),
            "total compacted bytes equal neighbor plus index bytes");

        const auto &partition = result.partitions.front();

        expect(
            partition.partition_index == 0U,
            "compacted partition retains source partition index");

        expect(
            partition.vertex_begin == partitions.front().vertex_begin(),
            "compacted partition retains vertex-begin boundary");

        expect(
            partition.vertex_end == partitions.front().vertex_end(),
            "compacted partition retains vertex-end boundary");

        expect(
            partition.active_vertices.size() == 1U,
            "one active vertex is retained");

        expect(
            partition.active_vertices.front() == 0U,
            "active vertex ID is preserved");

        expect(
            partition.neighbors.size() == 2U,
            "two neighbors are compacted");

        expect(
            partition.neighbors[0] == 1U,
            "first compacted neighbor is preserved");

        expect(
            partition.neighbors[1] == 2U,
            "second compacted neighbor is preserved");

        expect(
            partition.neighbor_index.size() == 2U,
            "compressed index has active vertex plus terminal entry");

        expect(
            partition.neighbor_index[0] == 0U,
            "compressed index starts at zero");

        expect(
            partition.neighbor_index[1] == 2U,
            "terminal compressed offset equals compacted edge count");

        expect(
            partition.active_edge_count() == 2U,
            "partition active edge count is correct");

        expect(
            partition.active_vertex_count() == 1U,
            "partition active vertex count is correct");

        const auto expanded =
            ExpTMCompaction::expanded_neighbors(partition);

        expect(
            expanded == partition.neighbors,
            "reference expansion reproduces compacted neighbor stream");
    }

    void test_exp_tm_compaction_multiple_active_vertices()
    {
        using hytgraph::graph::ActivityTracker;
        using hytgraph::graph::CSRGraph;
        using hytgraph::graph::LogicalPartitioner;
        using hytgraph::transfer::ExpTMCompaction;

        // Graph:
        //
        // 0 -> 1, 2
        // 1 -> 2
        // 2 -> 0, 3
        // 3 -> 1
        //
        // Activate vertices 0 and 2.
        //
        // Expected compacted edge stream:
        //
        // vertex 0: [1, 2]
        // vertex 2: [0, 3]
        //
        // neighbors = [1, 2, 0, 3]
        // index     = [0, 2, 4]

        CSRGraph graph(
            4,
            {0, 2, 3, 5, 6},
            {1, 2, 2, 0, 3, 1});

        LogicalPartitioner partitioner(
            16U * sizeof(CSRGraph::vertex_id));

        const auto partitions = partitioner.partition(graph);

        expect(
            partitions.size() == 1U,
            "large partition target keeps graph in one logical partition");

        ActivityTracker activity(graph.num_vertices());

        activity.set_active(0);
        activity.set_active(2);

        ExpTMCompaction compaction(true);

        const auto result =
            compaction.compact(graph, partitions, activity);

        expect(
            result.compacted_partition_count() == 1U,
            "active vertices in one partition produce one compacted partition");

        expect(
            result.active_vertex_count() == 2U,
            "two active source vertices are represented");

        expect(
            result.active_edge_count() == 4U,
            "two active source vertices contribute four edges");

        const auto &partition = result.partitions.front();

        expect(
            partition.active_vertices ==
                std::vector<CSRGraph::vertex_id>{0, 2},
            "active source vertices remain in ascending source order");

        expect(
            partition.neighbors ==
                std::vector<CSRGraph::vertex_id>{1, 2, 0, 3},
            "active neighbors are packed in source order");

        expect(
            partition.neighbor_index ==
                std::vector<CSRGraph::offset_type>{0, 2, 4},
            "compressed index contains the correct row boundaries");

        expect(
            partition.active_edge_count() ==
                static_cast<CSRGraph::offset_type>(
                    partition.neighbors.size()),
            "partition edge count equals compacted neighbor count");

        const auto expanded =
            ExpTMCompaction::expanded_neighbors(partition);

        expect(
            expanded ==
                std::vector<CSRGraph::vertex_id>{1, 2, 0, 3},
            "expanded compacted stream preserves all active neighbors");
    }

    void test_exp_tm_compaction_skips_inactive_partitions()
    {
        using hytgraph::graph::ActivityTracker;
        using hytgraph::graph::CSRGraph;
        using hytgraph::graph::LogicalPartitioner;
        using hytgraph::transfer::ExpTMCompaction;

        CSRGraph graph(
            4,
            {0, 2, 3, 4, 5},
            {1, 2, 2, 0, 0});

        LogicalPartitioner partitioner(
            2U * sizeof(CSRGraph::vertex_id));

        const auto partitions = partitioner.partition(graph);

        expect(
            partitions.size() > 1U,
            "small partition target produces multiple partitions");

        ActivityTracker activity(graph.num_vertices());

        // Activate a vertex in the first partition only.
        activity.set_active(0);

        ExpTMCompaction compaction(true);

        const auto result =
            compaction.compact(graph, partitions, activity);

        expect(
            result.compacted_partition_count() == 1U,
            "inactive partitions are omitted from compacted result");

        expect(
            result.partitions.front().partition_index == 0U,
            "the active partition is retained");

        expect(
            result.active_edge_count() == 2U,
            "only edges from the active source are compacted");

        expect(
            result.active_edge_count() <
                static_cast<CSRGraph::offset_type>(graph.num_edges()),
            "compaction removes inactive-source edge data");
    }

    void test_exp_tm_compaction_zero_degree_active_vertex()
    {
        using hytgraph::graph::ActivityTracker;
        using hytgraph::graph::CSRGraph;
        using hytgraph::graph::LogicalPartitioner;
        using hytgraph::transfer::ExpTMCompaction;

        // Vertex 1 is active but has no outgoing edges.
        CSRGraph graph(
            3,
            {0, 1, 1, 2},
            {1, 0});

        LogicalPartitioner partitioner(
            16U * sizeof(CSRGraph::vertex_id));

        const auto partitions = partitioner.partition(graph);

        ActivityTracker activity(graph.num_vertices());
        activity.set_active(1);

        ExpTMCompaction compaction(true);

        const auto result =
            compaction.compact(graph, partitions, activity);

        expect(
            result.compacted_partition_count() == 1U,
            "active zero-degree vertex still produces a compacted partition");

        expect(
            result.active_vertex_count() == 1U,
            "zero-degree active vertex is represented");

        expect(
            result.active_edge_count() == 0U,
            "zero-degree active vertex contributes no edges");

        const auto &partition = result.partitions.front();

        expect(
            partition.active_vertices ==
                std::vector<CSRGraph::vertex_id>{1},
            "zero-degree active vertex is retained");

        expect(
            partition.neighbors.empty(),
            "zero-degree active vertex has no compacted neighbors");

        expect(
            partition.neighbor_index ==
                std::vector<CSRGraph::offset_type>{0, 0},
            "zero-degree vertex receives an empty compressed range");

        const auto expanded =
            ExpTMCompaction::expanded_neighbors(partition);

        expect(
            expanded.empty(),
            "expansion of zero-degree compacted vertex is empty");
    }

    void test_exp_tm_compaction_all_active_preserves_edges()
    {
        using hytgraph::graph::ActivityTracker;
        using hytgraph::graph::CSRGraph;
        using hytgraph::graph::LogicalPartitioner;
        using hytgraph::transfer::ExpTMCompaction;

        CSRGraph graph(
            4,
            {0, 2, 3, 4, 5},
            {1, 2, 2, 0, 0});

        LogicalPartitioner partitioner(
            16U * sizeof(CSRGraph::vertex_id));

        const auto partitions = partitioner.partition(graph);

        ActivityTracker activity(graph.num_vertices());

        activity.set_active_vertices(
            std::vector<CSRGraph::vertex_id>{0, 1, 2, 3});

        expect(
            activity.active_edge_count(graph) ==
                static_cast<CSRGraph::offset_type>(graph.num_edges()),
            "all-active activity contains every graph edge");

        ExpTMCompaction compaction(true);

        const auto result =
            compaction.compact(graph, partitions, activity);

        expect(
            result.active_vertex_count() ==
                static_cast<CSRGraph::offset_type>(
                    graph.num_vertices()),
            "all graph vertices are represented");

        expect(
            result.active_edge_count() ==
                static_cast<CSRGraph::offset_type>(
                    graph.num_edges()),
            "all graph edges are preserved");

        expect(
            result.total_bytes() ==
                graph.num_edges() *
                        sizeof(CSRGraph::vertex_id) +
                    graph.num_vertices() *
                        sizeof(CSRGraph::offset_type) +
                    sizeof(CSRGraph::offset_type),
            "all-active compacted payload has expected neighbor and index size");

        const auto &partition = result.partitions.front();

        expect(
            partition.active_vertices ==
                std::vector<CSRGraph::vertex_id>{0, 1, 2, 3},
            "all active vertices are retained in source order");

        expect(
            partition.neighbors ==
                std::vector<CSRGraph::vertex_id>{
                    1, 2, 2, 0, 0},
            "all-active neighbor stream matches original CSR stream");

        expect(
            partition.neighbor_index ==
                std::vector<CSRGraph::offset_type>{
                    0, 2, 3, 4, 5},
            "all-active compressed index matches original CSR row boundaries");
    }

    void test_exp_tm_compaction_disabled_reference_path()
    {
        using hytgraph::graph::ActivityTracker;
        using hytgraph::graph::CSRGraph;
        using hytgraph::graph::LogicalPartitioner;
        using hytgraph::transfer::ExpTMCompaction;

        CSRGraph graph(
            3,
            {0, 1, 2, 2},
            {1, 2});

        LogicalPartitioner partitioner(
            2U * sizeof(CSRGraph::vertex_id));

        const auto partitions = partitioner.partition(graph);

        ActivityTracker activity(graph.num_vertices());
        activity.set_active(0);

        ExpTMCompaction compaction(false);

        expect(
            !compaction.enabled(),
            "disabled compaction reports disabled state");

        const auto result =
            compaction.compact(graph, partitions, activity);

        expect(
            result.partitions.empty(),
            "disabled compaction returns no compacted partitions");

        expect(
            result.compaction_seconds == 0.0,
            "disabled compaction does not report CPU compaction time");
    }

    void test_exp_tm_compaction_rejects_mismatched_activity()
    {
        using hytgraph::graph::ActivityTracker;
        using hytgraph::graph::CSRGraph;
        using hytgraph::graph::LogicalPartitioner;
        using hytgraph::transfer::ExpTMCompaction;

        CSRGraph graph(
            3,
            {0, 1, 2, 2},
            {1, 2});

        LogicalPartitioner partitioner(
            16U * sizeof(CSRGraph::vertex_id));

        const auto partitions = partitioner.partition(graph);

        ActivityTracker activity(2);

        ExpTMCompaction compaction(true);

        bool threw = false;

        try
        {
            (void)compaction.compact(
                graph,
                partitions,
                activity);
        }
        catch (const std::invalid_argument &)
        {
            threw = true;
        }

        expect(
            threw,
            "compaction rejects activity tracker with wrong vertex count");
    }

    void test_exp_tm_compaction_rejects_invalid_partition()
    {
        using hytgraph::graph::ActivityTracker;
        using hytgraph::graph::CSRGraph;
        using hytgraph::graph::LogicalPartition;
        using hytgraph::transfer::ExpTMCompaction;

        CSRGraph graph(
            3,
            {0, 1, 2, 2},
            {1, 2});

        ActivityTracker activity(graph.num_vertices());
        activity.set_active(0);

        // Deliberately construct a partition whose edge range does not
        // correspond to its CSR vertex range.
        LogicalPartition invalid_partition(
            0,
            2,
            1,
            2,
            1024);

        ExpTMCompaction compaction(true);

        bool threw = false;

        try
        {
            (void)compaction.compact(
                graph,
                {invalid_partition},
                activity);
        }
        catch (const std::invalid_argument &)
        {
            threw = true;
        }

        expect(
            threw,
            "compaction rejects partition with inconsistent CSR boundary");
    }
    void test_zero_copy_empty_activity()
    {
        using hytgraph::graph::ActivityTracker;
        using hytgraph::graph::CSRGraph;
        using hytgraph::graph::LogicalPartitioner;
        using hytgraph::transfer::ImpTMZeroCopy;
        using hytgraph::transfer::ZeroCopyMode;

        CSRGraph graph(
            4,
            {0, 2, 3, 4, 4},
            {1, 2, 2, 0});

        LogicalPartitioner partitioner(
            2U * sizeof(CSRGraph::vertex_id));

        const auto partitions = partitioner.partition(graph);

        ActivityTracker activity(graph.num_vertices());

        ImpTMZeroCopy zero_copy;

        const auto result =
            zero_copy.prepare(graph, partitions, activity);

        expect(result.mode == ZeroCopyMode::Modeled,
               "zero-copy reference path is modeled");

        expect(!result.host_memory_mapped,
               "modeled path does not claim host memory mapping");

        expect(result.active_partition_count == 0,
               "no active partitions for empty activity");

        expect(result.active_vertex_count == 0,
               "no active vertices for empty activity");

        expect(result.active_edge_count == 0,
               "no active edges for empty activity");

        expect(result.memory_request_count == 0,
               "no memory requests for empty activity");

        expect(result.alignment_overhead_count == 0,
               "no alignment overhead for empty activity");

        expect(result.modeled_tlp_count == 0,
               "no modeled TLPs for empty activity");
    }

    void test_zero_copy_active_vertices()
    {
        using hytgraph::graph::ActivityTracker;
        using hytgraph::graph::CSRGraph;
        using hytgraph::graph::LogicalPartitioner;
        using hytgraph::transfer::ImpTMZeroCopy;
        using hytgraph::transfer::ZeroCopyMode;

        // Graph:
        //
        // 0 -> 1, 2
        // 1 -> 2
        // 2 -> 0
        // 3 -> nothing
        //
        // Vertices 0 and 2 are active.
        //
        // Vertex 0:
        //   degree = 2
        //   neighbor payload = 2 * sizeof(vertex_id) = 8 bytes
        //   requests = ceil(8 / 128) = 1
        //
        // Vertex 2:
        //   degree = 1
        //   neighbor payload = 4 bytes
        //   requests = ceil(4 / 128) = 1
        //
        // Total:
        //   active vertices = 2
        //   active edges = 3
        //   memory requests = 2
        //   alignment overhead = 1
        //   modeled TLPs = ceil(2 / 256) = 1

        CSRGraph graph(
            4,
            {0, 2, 3, 4, 4},
            {1, 2, 2, 0});

        LogicalPartitioner partitioner(
            2U * sizeof(CSRGraph::vertex_id));

        const auto partitions = partitioner.partition(graph);

        ActivityTracker activity(graph.num_vertices());
        activity.set_active_vertices({0, 2});

        ImpTMZeroCopy zero_copy;

        const auto result =
            zero_copy.prepare(graph, partitions, activity);

        expect(result.mode == ZeroCopyMode::Modeled,
               "active zero-copy path remains modeled");

        expect(result.active_vertex_count == 2,
               "active vertex count");

        expect(result.active_edge_count == 3,
               "active edge count");

        expect(result.memory_request_count == 2,
               "memory request count");

        expect(result.alignment_overhead_count == 1,
               "unaligned logical CSR offset contributes alignment overhead");

        expect(result.modeled_tlp_count == 1,
               "two memory requests fit within one modeled TLP");

        expect(result.active_partition_count >= 1,
               "active vertices produce an active partition");

        std::size_t active_vertices_seen = 0;

        for (const auto &partition : result.partitions)
        {
            expect(partition.active_vertices.size() ==
                       partition.vertex_metrics.size(),
                   "active vertices and metrics have matching sizes");

            active_vertices_seen +=
                partition.active_vertices.size();

            for (const auto &metrics : partition.vertex_metrics)
            {
                expect(metrics.memory_requests >= 1,
                       "non-empty active adjacency has at least one request");

                expect(metrics.alignment_overhead <= 1,
                       "alignment overhead is binary");
            }
        }

        expect(active_vertices_seen == 2,
               "all active vertices appear exactly once");
    }

    void test_zero_copy_request_payload_configuration()
    {
        using hytgraph::graph::ActivityTracker;
        using hytgraph::graph::CSRGraph;
        using hytgraph::graph::LogicalPartitioner;
        using hytgraph::transfer::ImpTMZeroCopy;
        using hytgraph::transfer::ZeroCopyOptions;

        CSRGraph graph(
            2,
            {0, 40, 40},
            std::vector<CSRGraph::vertex_id>(
                40,
                1));

        LogicalPartitioner partitioner(
            1024U);

        const auto partitions = partitioner.partition(graph);

        ActivityTracker activity(graph.num_vertices());
        activity.set_active(0);

        ZeroCopyOptions options;
        options.request_payload_bytes = 16U;
        options.max_requests_per_tlp = 2U;

        ImpTMZeroCopy zero_copy(options);

        const auto result =
            zero_copy.prepare(graph, partitions, activity);

        // 40 vertex IDs * 4 bytes = 160 bytes.
        // With a 16-byte request payload:
        //
        //   ceil(160 / 16) = 10 requests
        //
        // With two requests per modeled TLP:
        //
        //   ceil(10 / 2) = 5 TLPs.
        expect(result.memory_request_count == 10,
               "configured request payload changes request count");

        expect(result.modeled_tlp_count == 5,
               "configured request concurrency changes TLP count");
    }

    void test_zero_copy_invalid_options()
    {
        using hytgraph::graph::ActivityTracker;
        using hytgraph::graph::CSRGraph;
        using hytgraph::graph::LogicalPartitioner;
        using hytgraph::transfer::ImpTMZeroCopy;
        using hytgraph::transfer::ZeroCopyOptions;

        CSRGraph graph(
            1,
            {0, 0},
            {});

        LogicalPartitioner partitioner(1024U);

        const auto partitions = partitioner.partition(graph);

        ActivityTracker activity(graph.num_vertices());

        {
            ZeroCopyOptions options;
            options.request_payload_bytes = 0U;

            ImpTMZeroCopy zero_copy(options);

            bool threw = false;

            try
            {
                (void)zero_copy.prepare(
                    graph,
                    partitions,
                    activity);
            }
            catch (const std::invalid_argument &)
            {
                threw = true;
            }

            expect(threw,
                   "zero-copy rejects zero request payload");
        }

        {
            ZeroCopyOptions options;
            options.max_requests_per_tlp = 0U;

            ImpTMZeroCopy zero_copy(options);

            bool threw = false;

            try
            {
                (void)zero_copy.prepare(
                    graph,
                    partitions,
                    activity);
            }
            catch (const std::invalid_argument &)
            {
                threw = true;
            }

            expect(threw,
                   "zero-copy rejects zero requests per TLP");
        }

        {
            ZeroCopyOptions options;
            options.alignment_bytes = 0U;

            ImpTMZeroCopy zero_copy(options);

            bool threw = false;

            try
            {
                (void)zero_copy.prepare(
                    graph,
                    partitions,
                    activity);
            }
            catch (const std::invalid_argument &)
            {
                threw = true;
            }

            expect(threw,
                   "zero-copy rejects zero alignment size");
        }
    }

    void test_zero_copy_partition_validation()
    {
        using hytgraph::graph::ActivityTracker;
        using hytgraph::graph::CSRGraph;
        using hytgraph::graph::LogicalPartition;
        using hytgraph::transfer::ImpTMZeroCopy;

        CSRGraph graph(
            3,
            {0, 1, 2, 2},
            {1, 2});

        ActivityTracker activity(graph.num_vertices());
        activity.set_active(0);

        // Deliberately omit vertex 2 from the supplied partition coverage.
        std::vector<LogicalPartition> invalid_partitions{
            LogicalPartition(
                0,
                2,
                0,
                2,
                2U * sizeof(CSRGraph::vertex_id))};

        ImpTMZeroCopy zero_copy;

        bool threw = false;

        try
        {
            (void)zero_copy.prepare(
                graph,
                invalid_partitions,
                activity);
        }
        catch (const std::invalid_argument &)
        {
            threw = true;
        }

        expect(threw,
               "zero-copy rejects incomplete logical partition coverage");
    }

    void test_zero_copy_zero_degree_active_vertex()
    {
        using hytgraph::graph::ActivityTracker;
        using hytgraph::graph::CSRGraph;
        using hytgraph::graph::LogicalPartitioner;
        using hytgraph::transfer::ImpTMZeroCopy;

        CSRGraph graph(
            3,
            {0, 1, 1, 1},
            {1});

        LogicalPartitioner partitioner(1024U);

        const auto partitions = partitioner.partition(graph);

        ActivityTracker activity(graph.num_vertices());
        activity.set_active(1);

        ImpTMZeroCopy zero_copy;

        const auto result =
            zero_copy.prepare(graph, partitions, activity);

        expect(result.active_vertex_count == 1,
               "zero-degree vertex remains an active vertex");

        expect(result.active_edge_count == 0,
               "zero-degree active vertex has no active edges");

        expect(result.memory_request_count == 0,
               "zero-degree active vertex has no neighbor requests");

        expect(result.alignment_overhead_count == 0,
               "zero-degree active vertex has no alignment overhead");

        expect(result.modeled_tlp_count == 0,
               "zero-degree active vertex requires no modeled TLP");
    }

    void test_hytm_cost_model()
    {
        using hytgraph::graph::ActivityTracker;
        using hytgraph::graph::CSRGraph;
        using hytgraph::graph::LogicalPartition;
        using hytgraph::transfer::HyTMCostModel;
        using hytgraph::transfer::HyTMCostModelOptions;

        CSRGraph graph(
            4,
            {0, 2, 3, 4, 4},
            {1, 2, 2, 0});

        ActivityTracker activity(graph.num_vertices());
        activity.set_active_vertices({0, 1});

        LogicalPartition partition(
            0,
            2,
            0,
            3,
            1024U);

        HyTMCostModelOptions options;
        options.alpha = 0.80;
        options.beta = 0.40;
        options.gamma = 0.625;
        options.request_payload_bytes = 128U;
        options.max_requests_per_tlp = 256U;
        options.rtt = 2.0;
        options.cpu_compaction_throughput_bytes_per_second = 14.0;

        HyTMCostModel model(options);

        const auto result =
            model.evaluate_partition(graph, partition, activity);

        expect(
            result.metrics.active_vertices == 2U,
            "HyTM active vertex count");

        expect(
            result.metrics.active_edges == 3U,
            "HyTM active edge count");

        expect(
            result.metrics.total_edges == 3U,
            "HyTM total edge count");

        // ExpTM-F:
        // 3 edges * 4 bytes = 12 bytes.
        // ceil(12 / (128 * 256)) = 1 TLP.
        // Cost = 1 * 2 = 2.
        expect(
            result.metrics.filter_transfer_bytes == 12U,
            "HyTM filter transfer bytes");

        expect(
            std::fabs(result.filter_cost - 2.0) < 1e-12,
            "HyTM filter cost");

        // ExpTM-C:
        // 3 * 4 destination bytes + 2 * 8 index bytes = 28 bytes.
        // Transfer = 2.
        // CPU = 28 / 14 = 2.
        // Total = 4.
        expect(
            result.metrics.compaction_bytes == 28U,
            "HyTM compaction bytes");

        expect(
            std::fabs(result.compaction_cost - 4.0) < 1e-12,
            "HyTM compaction cost");

        // ImpTM-ZC:
        // v0: ceil(2 * 4 / 128) = 1 request.
        // v1: ceil(1 * 4 / 128) = 1 request.
        //
        // v0 starts at logical byte offset 0 -> aligned.
        // v1 starts at logical byte offset 8 -> alignment overhead.
        expect(
            result.metrics.zero_copy_memory_requests == 2U,
            "HyTM zero-copy memory requests");

        expect(
            result.metrics.zero_copy_alignment_overhead == 1U,
            "HyTM zero-copy alignment overhead");

        expect(
            result.metrics.zero_copy_total_requests == 3U,
            "HyTM zero-copy total requests");

        // All partition edges are active, so RTTzc == RTT.
        expect(
            std::fabs(result.zero_copy_rtt - 2.0) < 1e-12,
            "HyTM zero-copy RTT");

        expect(
            std::fabs(result.zero_copy_cost - 2.0) < 1e-12,
            "HyTM zero-copy cost");
    }
    void test_hytm_selector()
    {
        using hytgraph::transfer::HyTMCostModel;
        using hytgraph::transfer::TransferEngine;

        // Compaction must satisfy both conditions strictly.
        expect(
            HyTMCostModel::select_engine(
                10.0,
                7.0,
                20.0,
                0.80,
                0.40) == TransferEngine::ExpTMCompaction,
            "HyTM selector chooses compaction");

        // 7.0 == 0.80 * 10.0 is false for strict '<'.
        // Filter is cheaper than zero-copy, so Filter wins.
        expect(
            HyTMCostModel::select_engine(
                10.0,
                8.0,
                20.0,
                0.80,
                0.40) == TransferEngine::ExpTMFilter,
            "HyTM selector uses strict alpha boundary");

        // Filter == ZeroCopy means Filter does not win.
        expect(
            HyTMCostModel::select_engine(
                10.0,
                20.0,
                10.0,
                0.80,
                0.40) == TransferEngine::ImpTMZeroCopy,
            "HyTM selector uses strict filter boundary");

        // Compaction == beta * ZeroCopy means compaction does not win.
        expect(
            HyTMCostModel::select_engine(
                20.0,
                8.0,
                20.0,
                0.80,
                0.40) == TransferEngine::ImpTMZeroCopy,
            "HyTM selector uses strict beta boundary");
    }
    void test_hytm_invalid_options()
    {
        using hytgraph::transfer::HyTMCostModel;
        using hytgraph::transfer::HyTMCostModelOptions;

        {
            auto options = HyTMCostModelOptions{};
            options.alpha = 0.0;

            bool threw = false;

            try
            {
                HyTMCostModel model(options);
                (void)model;
            }
            catch (const std::invalid_argument &)
            {
                threw = true;
            }

            expect(threw, "HyTM rejects non-positive alpha");
        }

        {
            auto options = HyTMCostModelOptions{};
            options.gamma = 1.1;

            bool threw = false;

            try
            {
                HyTMCostModel model(options);
                (void)model;
            }
            catch (const std::invalid_argument &)
            {
                threw = true;
            }

            expect(threw, "HyTM rejects gamma above one");
        }

        {
            auto options = HyTMCostModelOptions{};
            options.request_payload_bytes = 0U;

            bool threw = false;

            try
            {
                HyTMCostModel model(options);
                (void)model;
            }
            catch (const std::invalid_argument &)
            {
                threw = true;
            }

            expect(threw, "HyTM rejects zero request payload");
        }
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

        test_exp_tm_filter_skips_inactive_partitions();
        test_exp_tm_filter_full_partition_transfer();
        test_exp_tm_filter_disabled_reference_path();
        test_exp_tm_filter_rejects_mismatched_activity();

        test_exp_tm_compaction_basic();
        test_exp_tm_compaction_multiple_active_vertices();
        test_exp_tm_compaction_skips_inactive_partitions();
        test_exp_tm_compaction_zero_degree_active_vertex();
        test_exp_tm_compaction_all_active_preserves_edges();
        test_exp_tm_compaction_disabled_reference_path();
        test_exp_tm_compaction_rejects_mismatched_activity();
        test_exp_tm_compaction_rejects_invalid_partition();

        test_zero_copy_empty_activity();
        test_zero_copy_active_vertices();
        test_zero_copy_request_payload_configuration();
        test_zero_copy_invalid_options();
        test_zero_copy_partition_validation();
        test_zero_copy_zero_degree_active_vertex();

        test_hytm_cost_model();
        test_hytm_selector();
        test_hytm_invalid_options();

        std::cout << "All Phase 0 unit tests passed.\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
