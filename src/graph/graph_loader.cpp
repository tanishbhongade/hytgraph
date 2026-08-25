#include "graph/graph_loader.hpp"

#include <cstdint>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <algorithm>

namespace hytgraph::graph
{

    namespace
    {

        struct Edge
        {
            CSRGraph::vertex_id source;
            CSRGraph::vertex_id destination;
            CSRGraph::weight_type weight = 0.0F;
        };

        std::string trim(const std::string &value)
        {
            const std::size_t first = value.find_first_not_of(" \t\r\n");
            if (first == std::string::npos)
            {
                return {};
            }

            const std::size_t last = value.find_last_not_of(" \t\r\n");
            return value.substr(first, last - first + 1);
        }

    } // namespace

    CSRGraph GraphLoader::load_edge_list(
        const std::string &path,
        CSRGraph::offset_type num_vertices,
        bool weighted)
    {

        std::ifstream input(path);
        if (!input.is_open())
        {
            throw std::runtime_error(
                "GraphLoader::load_edge_list: unable to open file: " + path);
        }

        std::vector<Edge> edges;

        std::string line;
        std::size_t line_number = 0;

        while (std::getline(input, line))
        {
            ++line_number;

            const std::string stripped = trim(line);

            // Ignore empty lines and comments.
            if (stripped.empty() || stripped.front() == '#')
            {
                continue;
            }

            std::istringstream stream(stripped);

            std::uint64_t source_raw = 0;
            std::uint64_t destination_raw = 0;

            if (!(stream >> source_raw >> destination_raw))
            {
                throw std::invalid_argument(
                    "GraphLoader::load_edge_list: malformed edge at line " +
                    std::to_string(line_number));
            }

            if (source_raw >= num_vertices ||
                destination_raw >= num_vertices)
            {
                throw std::invalid_argument(
                    "GraphLoader::load_edge_list: vertex ID out of range at "
                    "line " +
                    std::to_string(line_number));
            }

            Edge edge;
            edge.source = static_cast<CSRGraph::vertex_id>(source_raw);
            edge.destination =
                static_cast<CSRGraph::vertex_id>(destination_raw);

            if (weighted)
            {
                if (!(stream >> edge.weight))
                {
                    throw std::invalid_argument(
                        "GraphLoader::load_edge_list: missing edge weight at "
                        "line " +
                        std::to_string(line_number));
                }
            }

            // Reject unexpected trailing tokens. This catches accidental
            // malformed records instead of silently ignoring input.
            std::string trailing;
            if (stream >> trailing)
            {
                throw std::invalid_argument(
                    "GraphLoader::load_edge_list: unexpected data at line " +
                    std::to_string(line_number));
            }

            edges.push_back(edge);
        }

        if (input.bad())
        {
            throw std::runtime_error(
                "GraphLoader::load_edge_list: error while reading file: " + path);
        }

        // CSR requires edges to be grouped by source vertex.
        //
        // std::stable_sort preserves the original ordering of edges having
        // the same source. We intentionally do not sort by destination and
        // do not deduplicate parallel edges.
        std::stable_sort(
            edges.begin(),
            edges.end(),
            [](const Edge &lhs, const Edge &rhs)
            {
                return lhs.source < rhs.source;
            });

        std::vector<CSRGraph::offset_type> row_offsets(
            static_cast<std::size_t>(num_vertices) + 1U,
            0);

        for (const Edge &edge : edges)
        {
            ++row_offsets[static_cast<std::size_t>(edge.source) + 1U];
        }

        // Convert per-vertex edge counts into CSR prefix sums.
        for (std::size_t vertex = 0; vertex < static_cast<std::size_t>(num_vertices);
             ++vertex)
        {
            row_offsets[vertex + 1U] += row_offsets[vertex];
        }

        std::vector<CSRGraph::vertex_id> column_indices;
        column_indices.reserve(edges.size());

        std::vector<CSRGraph::weight_type> edge_weights;

        if (weighted)
        {
            edge_weights.reserve(edges.size());
        }

        for (const Edge &edge : edges)
        {
            column_indices.push_back(edge.destination);

            if (weighted)
            {
                edge_weights.push_back(edge.weight);
            }
        }

        return CSRGraph(
            num_vertices,
            std::move(row_offsets),
            std::move(column_indices),
            std::move(edge_weights));
    }

} // namespace hytgraph::graph