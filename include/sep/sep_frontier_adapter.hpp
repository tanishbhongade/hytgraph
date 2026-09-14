#ifndef HYTGRAPH_SEP_FRONTIER_ADAPTER_HPP
#define HYTGRAPH_SEP_FRONTIER_ADAPTER_HPP

#include <cstddef>
#include <utility>
#include <vector>

#include "sep_frontier.hpp"

namespace hytgraph::sep
{

    /**
     * Adapter for an existing project-owned frontier/worklist.
     *
     * The wrapped type is not required to inherit from SEPFrontier.
     * It only needs to provide the small set of operations required by this
     * adapter:
     *
     *   clear()
     *   push(node)
     *   pop(node) -> bool
     *   size() -> std::size_t
     *   empty() -> bool
     *
     * This keeps SEP execution independent from the concrete representation
     * already used by the project.
     *
     * The adapter is non-owning.
     */
    template <typename Frontier>
    class SEPFrontierAdapter final : public SEPFrontier
    {
    public:
        using node_id_type = SEPFrontier::node_id_type;
        using frontier_type = Frontier;

        explicit SEPFrontierAdapter(Frontier &frontier) noexcept
            : frontier_(&frontier) {}

        void clear() noexcept override
        {
            frontier_->clear();
        }

        void push(node_id_type node) override
        {
            frontier_->push(node);
        }

        bool pop(node_id_type &node) override
        {
            return frontier_->pop(node);
        }

        std::size_t size() const noexcept override
        {
            return frontier_->size();
        }

        bool empty() const noexcept override
        {
            return frontier_->empty();
        }

        /**
         * Existing project frontier remains the owner.
         */
        Frontier &underlying() noexcept
        {
            return *frontier_;
        }

        const Frontier &underlying() const noexcept
        {
            return *frontier_;
        }

        /**
         * The adapter itself does not impose deterministic ordering.
         *
         * A concrete project frontier can override this property by providing
         * its own SEPFrontier implementation when deterministic traversal is
         * required by a test or future execution path.
         */
        bool is_deterministic() const noexcept override
        {
            return false;
        }

    private:
        Frontier *frontier_;
    };

    /**
     * Factory helper for constructing a non-owning adapter.
     */
    template <typename Frontier>
    SEPFrontierAdapter<Frontier> make_sep_frontier_adapter(
        Frontier &frontier) noexcept
    {
        return SEPFrontierAdapter<Frontier>(frontier);
    }

} // namespace hytgraph::sep

#endif // HYTGRAPH_SEP_FRONTIER_ADAPTER_HPP