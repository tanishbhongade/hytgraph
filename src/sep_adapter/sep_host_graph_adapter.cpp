// src/sep_adapter/sep_host_graph_adapter.cpp
//
// Phase 14 — Data-movement bridge.
//
// Serializes a project-owned CSRGraph to a temporary text file in
// the only format the vendored parser actually accepts:
// ReadGraphMarket_bigdata ("market_big"), one "src dst [weight]"
// per line, 0-indexed, whitespace-separated, no header.
//
// See sep_host_graph_adapter.hpp for the design rationale.

#include "sep_adapter/sep_host_graph_adapter.hpp"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <random>
#include <system_error>

namespace hytgraph
{
    namespace sep_adapter
    {
        using hytgraph::graph::CSRGraph;
        namespace
        {

            constexpr const char *kMarketBigFormatFlag = "market_big";
            constexpr int kIntegerWeightNum = 0;

            std::string make_unique_basename()
            {
                static std::mt19937_64 rng{std::random_device{}()};
                std::uniform_int_distribution<std::uint64_t> dist;
                char buf[40];
                std::snprintf(buf, sizeof(buf), "hytgraph_sep_%016llx",
                              static_cast<unsigned long long>(dist(rng)));
                return std::string(buf);
            }

            std::filesystem::path
            resolve_path(const SEPGraphFileConfig &config)
            {
                std::filesystem::path dir;
                if (config.directory.empty())
                {
                    dir = std::filesystem::temp_directory_path();
                }
                else
                {
                    dir = config.directory;
                }
                const std::string base = config.base_name.empty()
                                             ? make_unique_basename()
                                             : config.base_name;
                return dir / (base + ".txt");
            }

            // Vendored parser stores weights as uint32. Project CSR uses float.
            // Scale, round, and clamp to a minimum of 1 (0 is legal but a
            // zero-weight edge is indistinguishable from a missing one in some
            // algorithm paths).
            std::uint32_t scale_weight(float w, float scale)
            {
                if (!(scale > 0.0f))
                    scale = 1.0f;
                float scaled = w * scale;
                if (!std::isfinite(scaled) || scaled < 1.0f)
                    scaled = 1.0f;
                const long long rounded = std::llround(scaled);
                if (rounded > static_cast<long long>(0xFFFFFFFFu))
                {
                    return 0xFFFFFFFFu;
                }
                return static_cast<std::uint32_t>(rounded);
            }

        } // namespace

        // ---------------------------------------------------------------------------
        // SEPGraphFile
        // ---------------------------------------------------------------------------

        SEPGraphFile::SEPGraphFile(std::string path,
                                   SEPGraphFileFormat format,
                                   bool weighted) noexcept
            : path_(std::move(path)),
              format_(format),
              weighted_(weighted) {}

        SEPGraphFile::~SEPGraphFile() { reset(); }

        SEPGraphFile::SEPGraphFile(SEPGraphFile &&other) noexcept
            : path_(std::move(other.path_)),
              format_(other.format_),
              weighted_(other.weighted_)
        {
            other.path_.clear();
            other.weighted_ = false;
        }

        SEPGraphFile &SEPGraphFile::operator=(SEPGraphFile &&other) noexcept
        {
            if (this != &other)
            {
                reset();
                path_ = std::move(other.path_);
                format_ = other.format_;
                weighted_ = other.weighted_;
                other.path_.clear();
                other.weighted_ = false;
            }
            return *this;
        }

        bool SEPGraphFile::valid() const noexcept
        {
            if (path_.empty())
                return false;
            std::error_code ec;
            return std::filesystem::exists(path_, ec) && !ec;
        }

        const std::string &SEPGraphFile::path() const noexcept { return path_; }

        const char *SEPGraphFile::format_flag_value() const noexcept
        {
            (void)format_; // see header comment
            return kMarketBigFormatFlag;
        }

        int SEPGraphFile::weight_num_flag_value() const noexcept
        {
            return kIntegerWeightNum;
        }

        bool SEPGraphFile::weighted() const noexcept { return weighted_; }

        void SEPGraphFile::reset() noexcept
        {
            if (!path_.empty())
            {
                std::error_code ec;
                std::filesystem::remove(path_, ec);
                path_.clear();
            }
            weighted_ = false;
        }

        // ---------------------------------------------------------------------------
        // Factory
        // ---------------------------------------------------------------------------

        std::optional<SEPGraphFile>
        write_sep_graph_file(const CSRGraph &graph,
                             const SEPGraphFileConfig &config)
        {
            const auto nv = graph.num_vertices();
            if (nv == 0)
                return std::nullopt;

            const bool weighted = config.weighted && graph.has_weights();
            if (config.weighted && !graph.has_weights())
            {
                return std::nullopt;
            }

            try
            {
                graph.validate();
            }
            catch (...)
            {
                return std::nullopt;
            }

            // Vendored parser precondition: the highest vertex must appear
            // as a source, or xadj_pri undersizes and the parser overruns.
            // See the parser body of ReadGraphMarket_bigdata.
            const auto highest = static_cast<CSRGraph::vertex_id>(nv - 1);
            if (graph.out_degree(highest) == 0)
            {
                return std::nullopt;
            }

            std::filesystem::path path;
            try
            {
                path = resolve_path(config);
            }
            catch (...)
            {
                return std::nullopt;
            }

            std::ofstream out(path, std::ios::binary | std::ios::trunc);
            if (!out)
                return std::nullopt;

            const auto &offsets = graph.row_offsets();
            const auto &columns = graph.column_indices();
            const auto &weights = graph.edge_weights();

            for (CSRGraph::offset_type v = 0; v < nv; ++v)
            {
                const auto begin = offsets[v];
                const auto end = offsets[v + 1];
                for (auto e = begin; e < end; ++e)
                {
                    const auto dst = columns[e];
                    if (weighted)
                    {
                        out << v << ' ' << dst << ' '
                            << scale_weight(weights[e], config.weight_scale)
                            << '\n';
                    }
                    else
                    {
                        // Placeholder weight 1. Parser only reads the third
                        // column when weight_num != 1; we always set 0.
                        out << v << ' ' << dst << " 1\n";
                    }
                }
            }

            out.flush();
            if (!out)
            {
                out.close();
                std::error_code ec;
                std::filesystem::remove(path, ec);
                return std::nullopt;
            }
            out.close();

            return SEPGraphFile{path.string(), config.format, weighted};
        }

    } // namespace sep_adapter
} // namespace hytgraph