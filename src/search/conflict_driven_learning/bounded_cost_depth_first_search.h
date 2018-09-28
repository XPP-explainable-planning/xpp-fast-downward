#ifndef BOUNDED_COST_DEPTH_FIRST_SEARCH_H
#define BOUNDED_COST_DEPTH_FIRST_SEARCH_H

#include "../search_engine.h"
#include "../per_state_information.h"
#include "../operator_id.h"
#include "../state_id.h"
#include "../global_state.h"
#include "../evaluator.h"
#include "heuristic_refiner.h"

#include <map>
#include <deque>

namespace conflict_driven_learning {
namespace bounded_cost {

class BoundedCostDepthFirstSearch : public SearchEngine 
{
public:
    BoundedCostDepthFirstSearch(const options::Options& opts);
    virtual ~BoundedCostDepthFirstSearch() = default;
    virtual void print_statistics() const override;
    static void add_options_to_parser(options::OptionParser& parser);

protected:
    virtual void initialize() override;
    virtual SearchStatus step() override;
    bool evaluate(const GlobalState& state, Evaluator* eval);
    bool expand(const GlobalState& state);

    struct Locals {
        GlobalState state;
        OperatorID successor_op;
        std::map<int, std::deque<std::pair<OperatorID, StateID> > > open;
        std::vector<std::pair<int, GlobalState> > successors;
        Locals(const GlobalState& state);
    };

    bool c_refinement_toggle;

    std::shared_ptr<AbstractTask> m_task;
    TaskProxy m_task_proxy;

    Evaluator* m_expansion_evaluator;
    Evaluator* m_pruning_evaluator;
    std::shared_ptr<HeuristicRefiner> m_refiner;

    PerStateInformation<int> m_bounds;
    std::deque<Locals> m_call_stack;
    bool m_solved;
    Plan m_plan;

    int m_cached_h;
    int m_current_g;
};

}
}


#endif
