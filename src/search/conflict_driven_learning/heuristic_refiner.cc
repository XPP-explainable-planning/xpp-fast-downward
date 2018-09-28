#include "heuristic_refiner.h"

#include "../plugin.h"

namespace conflict_driven_learning
{

HeuristicRefiner::HeuristicRefiner()
{
    m_refinement_timer.stop();
    m_refinement_timer.reset();
    m_initialized = false;
}

void HeuristicRefiner::initialize()
{
}

void HeuristicRefiner::print_statistics() const
{
}

bool HeuristicRefiner::notify(
        int bound,
        StateComponent& component,
        SuccessorComponent& recognized_neighbors)
{
    if (!m_initialized) {
        m_initialized = true;
        initialize();
    }
    m_refinement_timer.resume();
    bool res = refine_heuristic(bound, component, recognized_neighbors);
    m_refinement_timer.stop();
    return res;
}

bool HeuristicRefiner::notify(
        int bound,
        StateComponent&& component,
        SuccessorComponent&& recognized_neighbors)
{
    return notify(bound, component, recognized_neighbors);
}

//bool HeuristicRefiner::notify(
//        int bound,
//        StateComponent &&component,
//        std::vector<std::pair<int, GlobalState> > &recognized_neighbors)
//{
//    return notify(bound, component, recognized_neighbors);
//}

const utils::Timer& HeuristicRefiner::get_refinement_timer() const
{
    return m_refinement_timer;
}

}

static PluginTypePlugin<conflict_driven_learning::HeuristicRefiner> _plugin_type(
        "HeuristicRefiner", "");



