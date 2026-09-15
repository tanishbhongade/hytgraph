// include/sep_adapter/sep_host_graph_adapter.hpp
//
// Phase 14 — Data-movement bridge.
//
// The vendored sepgraph::engine::Engine obtains its host graph from a
// file via utils::traversal::Context's constructor, which calls
// GetCachedGraph(FLAGS_graphfile, FLAGS_format, ...). There is no
// public API to inject an in-memory host graph.
//
// This adapter serializes a project-owned CSRGraph to a temporary
// file in a format the vendored parser accepts, and reports the
// path plus the FLAGS_format value the caller must set before
// constructing the Engine.
//
// This file replaces the MASTER_PLAN §Phase 14 "sep_graph_datum_
// adapter", which assumed in-memory construction of a
// groute::graphs::host::CSRGraph. That plan conflicts with the
// vendored engine's actual interface. Documented in the Phase 14
// handoff under "Approximations" and "Known Issues".
//
// PUBLIC BOUNDARY: This header does NOT include any vendored header
// and exposes no vendored type. It depends only on the project-owned
// CSRGraph and standard library types.

#pragma once

#include <optional>
#include <string>

#include "graph/csr_graph.hpp"

namespace hytgraph
{
    namespace sep_adapter
    {

        using hytgraph::graph::CSRGraph;
        // ---------------------------------------------------------------------------
        // Configuration
        // ---------------------------------------------------------------------------

        enum class SEPGraphFileFormat : std::uint8_t
        {
            // Vendored ReadGraphMarket_bigdata layout. Text file,
            // one "src dst [weight]" per line, 0-indexed, no header.
            // This is the only functional vendored parser.
            MatrixMarket = 0,

            // Reserved. The vendored ReadGraphGR is a stub in this
            // snapshot; do not use.
            GrouteNative = 1,
        };

        struct SEPGraphFileConfig
        {
            SEPGraphFileFormat format = SEPGraphFileFormat::MatrixMarket;

            // If true, project CSR edge_weights are written into the file.
            // If false, the file is written unweighted (pattern format).
            //
            // PageRank uses NoWeight and must set this to false.
            // SSSP requires weighted and must set this to true.
            bool weighted = false;

            // Vendored graph_t::adjwgt is idx_t (uint32). Project CSR uses
            // float. Non-integer float weights must be scaled and rounded.
            //
            // The written integer weight is:
            //     max(1, static_cast<uint32_t>(round(w * weight_scale)))
            //
            // Choose weight_scale to preserve the precision your algorithm
            // requires. For unit-weight graphs, weight_scale = 1.0 works.
            //
            // Ignored when `weighted == false`.
            float weight_scale = 1.0f;

            // Directory for the temporary file. If empty, uses
            // std::filesystem::temp_directory_path().
            std::string directory;

            // Optional base name (without extension). If empty, a unique
            // name is generated.
            std::string base_name;
        };

        // ---------------------------------------------------------------------------
        // Owned temp-file handle
        // ---------------------------------------------------------------------------
        //
        // A SEPGraphFile owns a temporary file on disk and removes it on
        // destruction. Move-only. Default-constructed instances are "invalid"
        // and hold no resources.

        class SEPGraphFile
        {
        public:
            SEPGraphFile() = default;
            ~SEPGraphFile();

            SEPGraphFile(SEPGraphFile &&other) noexcept;
            SEPGraphFile &operator=(SEPGraphFile &&other) noexcept;

            SEPGraphFile(const SEPGraphFile &) = delete;
            SEPGraphFile &operator=(const SEPGraphFile &) = delete;

            // True if the file was written successfully and still exists.
            bool valid() const noexcept;

            // Absolute path to the temporary file. Empty if !valid().
            const std::string &path() const noexcept;

            // Value the caller must assign to FLAGS_format before
            // constructing the vendored Engine. Never null.
            //
            // Confirmed against utils/utils.cpp::GetCachedGraph and
            // utils/parser.cpp in the vendored snapshot:
            //
            //   "market_big" -> ReadGraphMarket_bigdata  (functional)
            //   "market"     -> ReadGraphMarket          (stub)
            //   "metis"      -> ReadGraph                (stub)
            //   "gr"         -> ReadGraphGR              (stub)
            //
            // Only ReadGraphMarket_bigdata is functional, so this
            // method always returns "market_big" regardless of the
            // SEPGraphFileFormat value. The enum is preserved for
            // future use if the other parsers are ever implemented.
            const char *format_flag_value() const noexcept;

            // Value the caller must assign to FLAGS_weight_num before
            // constructing the vendored Engine.
            //
            // ReadGraphMarket_bigdata interprets this as:
            //   1  -> synthetic weights (src % 64), third column ignored
            //   !1 -> integer weights read from the third column
            //
            // This adapter always writes integer weights (or a
            // placeholder 1), so it always returns 0.
            int weight_num_flag_value() const noexcept;

            // Whether the file was written with edge weights.
            bool weighted() const noexcept;

            // Explicitly remove the file. Called automatically by the
            // destructor. Idempotent.
            void reset() noexcept;

        private:
            friend std::optional<SEPGraphFile>
            write_sep_graph_file(const CSRGraph &, const SEPGraphFileConfig &);

            explicit SEPGraphFile(std::string path,
                                  SEPGraphFileFormat format,
                                  bool weighted) noexcept;

            std::string path_;
            SEPGraphFileFormat format_ = SEPGraphFileFormat::MatrixMarket;
            bool weighted_ = false;
        };

        // ---------------------------------------------------------------------------
        // Factory
        // ---------------------------------------------------------------------------
        //
        // Writes a temporary file from the given CSR. Returns std::nullopt if:
        //   * the CSR is empty (num_vertices == 0);
        //   * the CSR fails validation (out-of-range destination IDs,
        //     non-monotonic offsets);
        //   * `config.weighted` is true but the CSR has no weights;
        //   * the file cannot be created or written.
        //
        // The returned object owns the file. The caller is responsible for
        // keeping it alive as long as the vendored Engine will read it.

        std::optional<SEPGraphFile>
        write_sep_graph_file(const CSRGraph &graph,
                             const SEPGraphFileConfig &config = {});

    } // namespace sep_adapter
} // namespace hytgraph