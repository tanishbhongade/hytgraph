#include <cassert>
#include <cstddef>
#include <deque>

#include "sep/sep_frontier.hpp"

namespace
{

    using namespace hytgraph::sep;

    /**
     * Minimal FIFO implementation used only to test the SEPFrontier contract.
     *
     * This is intentionally a test double. It does not represent the concrete
     * SEP scheduler/worklist implementation, which belongs to later execution
     * phases.
     */
    class TestFrontier final : public SEPFrontier
    {
    public:
        void clear() noexcept override
        {
            nodes_.clear();
        }

        void push(node_id_type node) override
        {
            nodes_.push_back(node);
        }

        bool pop(node_id_type &node) override
        {
            if (nodes_.empty())
            {
                return false;
            }

            node = nodes_.front();
            nodes_.pop_front();

            return true;
        }

        std::size_t size() const noexcept override
        {
            return nodes_.size();
        }

        bool empty() const noexcept override
        {
            return nodes_.empty();
        }

    private:
        std::deque<node_id_type> nodes_;
    };

    void test_default_frontier()
    {
        TestFrontier frontier;

        assert(frontier.empty());
        assert(frontier.size() == 0);
    }

    void test_push_and_pop()
    {
        TestFrontier frontier;

        frontier.push(10);
        frontier.push(20);
        frontier.push(30);

        assert(!frontier.empty());
        assert(frontier.size() == 3);

        SEPFrontier::node_id_type node = 0;

        assert(frontier.pop(node));
        assert(node == 10);

        assert(frontier.pop(node));
        assert(node == 20);

        assert(frontier.pop(node));
        assert(node == 30);

        assert(frontier.empty());
        assert(frontier.size() == 0);
    }

    void test_duplicate_entries()
    {
        TestFrontier frontier;

        frontier.push(42);
        frontier.push(42);

        /*
         * SEPFrontier itself does not require deduplication.
         * Concrete schedulers may impose their own policy.
         */
        assert(frontier.size() == 2);

        SEPFrontier::node_id_type node = 0;

        assert(frontier.pop(node));
        assert(node == 42);

        assert(frontier.pop(node));
        assert(node == 42);
    }

    void test_clear()
    {
        TestFrontier frontier;

        frontier.push(1);
        frontier.push(2);
        frontier.push(3);

        assert(frontier.size() == 3);

        frontier.clear();

        assert(frontier.empty());
        assert(frontier.size() == 0);
    }

    void test_reuse_after_clear()
    {
        TestFrontier frontier;

        frontier.push(1);
        frontier.push(2);

        frontier.clear();

        frontier.push(100);

        assert(frontier.size() == 1);

        SEPFrontier::node_id_type node = 0;

        assert(frontier.pop(node));
        assert(node == 100);

        assert(frontier.empty());
    }

    void test_fifo_order_is_not_part_of_base_contract()
    {
        /*
         * The base SEPFrontier contract only specifies push/pop behavior.
         * This test verifies our test implementation's FIFO behavior, while
         * explicitly keeping the ordering policy out of the production
         * interface.
         */
        TestFrontier frontier;

        frontier.push(1);
        frontier.push(2);
        frontier.push(3);

        SEPFrontier::node_id_type node = 0;

        assert(frontier.pop(node));
        assert(node == 1);

        assert(frontier.pop(node));
        assert(node == 2);

        assert(frontier.pop(node));
        assert(node == 3);
    }

    void test_empty_pop()
    {
        TestFrontier frontier;

        SEPFrontier::node_id_type node = 1234;

        assert(!frontier.pop(node));

        /*
         * The failed pop must not fabricate a successful work item.
         */
        assert(frontier.empty());
        assert(frontier.size() == 0);
    }

    void test_polymorphic_interface()
    {
        TestFrontier concrete;

        SEPFrontier &frontier = concrete;

        frontier.push(7);
        frontier.push(8);

        assert(frontier.size() == 2);
        assert(!frontier.empty());

        SEPFrontier::node_id_type node = 0;

        assert(frontier.pop(node));
        assert(node == 7);

        assert(frontier.pop(node));
        assert(node == 8);

        assert(!frontier.pop(node));
        assert(frontier.empty());
    }

} // namespace

int main()
{
    test_default_frontier();
    test_push_and_pop();
    test_duplicate_entries();
    test_clear();
    test_reuse_after_clear();
    test_fifo_order_is_not_part_of_base_contract();
    test_empty_pop();
    test_polymorphic_interface();

    return 0;
}