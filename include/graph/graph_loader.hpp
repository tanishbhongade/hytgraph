#pragma once

#include "graph/csr_graph.hpp"

#include <string>

namespace hytgraph::graph
{

    class GraphLoader
    {
    public:
        // Load an edge-list graph.
        //
        // Each non-empty, non-comment line must contain:
        //
        //     source destination
        //
        // for an unweighted graph, or:
        //
        //     source destination weight
        //
        // for a weighted graph.
        //
        // Vertex IDs are expected to be zero-based and in the range
        // [0, num_vertices).
        //
        // The loader constructs CSRGraph and therefore performs the same
        // structural validation as a directly constructed CSRGraph.
        [[nodiscard]] static CSRGraph load_edge_list(
            const std::string &path,
            CSRGraph::offset_type num_vertices,
            bool weighted = false);
    };

} // namespace hytgraph::graph