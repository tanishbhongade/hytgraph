#include "sep/sep_application.hpp"
#include "sep/sep_application_adapter.hpp"
#include "sep/sep_application_traits.hpp"

#include "sep/sep_execution_config.hpp"
#include "sep/sep_execution_context.hpp"
#include "sep/sep_execution_driver.hpp"
#include "sep/sep_execution_driver_base.hpp"
#include "sep/sep_execution_factory.hpp"
#include "sep/sep_execution_plan.hpp"
#include "sep/sep_execution_requirements.hpp"
#include "sep/sep_execution_result.hpp"
#include "sep/sep_execution_selection.hpp"
#include "sep/sep_execution_variant.hpp"
#include "sep/sep_execution_variant_registry.hpp"

#include "sep/sep_frontier.hpp"
#include "sep/sep_frontier_adapter.hpp"

int main()
{
    /*
     * This translation unit intentionally contains no execution logic.
     *
     * Its purpose is to catch:
     *   - missing direct includes;
     *   - circular include problems;
     *   - declaration-order problems;
     *   - incompatible public SEP Phase-12 interfaces.
     *
     * Actual GPU execution remains deferred to Phase 13.
     */
    return 0;
}