#ifndef HYTGRAPH_SEP_FRONTIER_HPP
#define HYTGRAPH_SEP_FRONTIER_HPP

#include <cstddef>
#include <vector>

namespace hytgraph::sep
{

    /**
     * Logical work frontier used by SEP data-driven execution.
     *
     * SEP-Graph's DD variants operate on active vertices represented through
     * queue/worklist structures. This project-local interface exposes only the
     * operations needed by a future SEP execution driver.
     *
     * IMPORTANT:
     *
     * This is an execution boundary, not a replacement for the project's
     * existing activity/worklist implementation.
     *
     * A future adapter can make an existing project activity tracker or
     * worklist satisfy this interface without changing that implementation.
     *
     * Phase 12 does not provide a CUDA/device frontier.
     */
    class SEPFrontier
    {
    public:
        using node_id_type = std::size_t;

        virtual ~SEPFrontier() = default;

        /**
         * Remove all entries from the logical frontier.
         */
        virtual void clear() noexcept = 0;

        /**
         * Add one vertex to the frontier.
         *
         * Duplicate handling is intentionally left to the concrete frontier.
         * Some SEP-style implementations use queue semantics while others use
         * bitmap-assisted duplicate suppression.
         */
        virtual void push(node_id_type node) = 0;

        /**
         * Add a batch of vertices to the frontier.
         */
        virtual void push(
            const std::vector<node_id_type> &nodes)
        {
            for (const node_id_type node : nodes)
            {
                push(node);
            }
        }

        /**
         * Remove one vertex from the frontier.
         *
         * Returns false when the frontier is empty.
         */
        virtual bool pop(node_id_type &node) = 0;

        /**
         * Number of currently available logical work items.
         */
        virtual std::size_t size() const noexcept = 0;

        /**
         * Whether there is no available logical work.
         */
        virtual bool empty() const noexcept = 0;

        /**
         * Whether this frontier provides deterministic iteration/order.
         *
         * The default is false because queue/worklist implementations are not
         * required to preserve a deterministic order by the SEP execution model.
         *
         * This method exists so tests and future execution code can explicitly
         * distinguish a deterministic project adapter from an unordered GPU
         * worklist.
         */
        virtual bool is_deterministic() const noexcept
        {
            return false;
        }
    };

    /**
     * Non-owning description of a DD execution frontier.
     *
     * This wrapper allows the execution driver to distinguish the absence of
     * DD work from the frontier object itself without taking ownership of it.
     *
     * No graph state is stored here.
     */
    class SEPFrontierView
    {
    public:
        explicit SEPFrontierView(SEPFrontier *frontier) noexcept
            : frontier_(frontier) {}

        SEPFrontier *get() noexcept
        {
            return frontier_;
        }

        const SEPFrontier *get() const noexcept
        {
            return frontier_;
        }

        bool valid() const noexcept
        {
            return frontier_ != nullptr;
        }

        std::size_t size() const noexcept
        {
            return frontier_ == nullptr ? 0 : frontier_->size();
        }

        bool empty() const noexcept
        {
            return frontier_ == nullptr || frontier_->empty();
        }

    private:
        SEPFrontier *frontier_;
    };

} // namespace hytgraph::sep

#endif // HYTGRAPH_SEP_FRONTIER_HPP