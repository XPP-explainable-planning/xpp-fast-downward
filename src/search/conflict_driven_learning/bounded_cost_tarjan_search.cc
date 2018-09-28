#include "bounded_cost_tarjan_search.h"
#include "state_component.h"

#include "../evaluation_context.h"
#include "../evaluation_result.h"
#include "../globals.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../state_registry.h"
#include "../task_utils/successor_generator.h"
#include "../task_utils/task_properties.h"
#include "../utils/system.h"
#include "../operator_cost.h"
#include "../tasks/cost_adapted_task.h"


#include "hc_heuristic.h"

#include <limits>
#include <algorithm>

#ifndef NDEBUG
#define DEBUG_BOUNDED_COST_DFS_ASSERT_LEARNING 1
#define DEBUG_BOUNDED_COST_DFS_ASSERT_NEIGHBORS 1
#endif

namespace conflict_driven_learning {
namespace bounded_cost {

static const int INF = std::numeric_limits<int>::max();

BoundedCostTarjanSearch::Locals::Locals(const GlobalState& state, bool zero_layer, unsigned size)
    : state(state), successor_op(OperatorID::no_operator), zero_layer(zero_layer), neighbors_size(size)
{}

BoundedCostTarjanSearch::ExpansionInfo::ExpansionInfo()
    : index(INF), lowlink(INF) {}

BoundedCostTarjanSearch::ExpansionInfo&
BoundedCostTarjanSearch::HashMap::operator[](const StateID& state)
{
    return data[state];
}

void
BoundedCostTarjanSearch::HashMap::remove(const StateID& state)
{
    data.erase(data.find(state));
}

BoundedCostTarjanSearch::PerLayerData::PerLayerData()
    : index(0) {}

BoundedCostTarjanSearch::BoundedCostTarjanSearch(const options::Options& opts)
    : SearchEngine(opts)
    , c_refinement_toggle(false)
    , c_compute_neighbors(false)
    , c_make_neighbors_unique(opts.get<bool>("unique_neighbors"))
    , m_task(OperatorCost(opts.get_enum("cost_type")) != OperatorCost::NORMAL ? std::make_shared<tasks::CostAdaptedTask>(opts) : task)
    , m_task_proxy(*m_task)
    , m_expansion_evaluator(opts.get<Evaluator *>("eval"))
    , m_pruning_evaluator(NULL)
    , m_refiner(opts.contains("learn") ? opts.get<std::shared_ptr<HeuristicRefiner> >("learn") : nullptr)
    , m_bounds(-1)
    , m_solved(false)
    , m_last_state(StateID::no_state)
{
    if (m_refiner != nullptr) {
        c_refinement_toggle = true;
        c_compute_neighbors = true;
        m_pruning_evaluator = m_refiner->get_underlying_heuristic().get();
    }

    if (bound == std::numeric_limits<int>::max()) {
        std::cerr << "bounded cost depth first search requires bound < infinity"
                    << std::endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
    m_current_g = 0;
}

void
BoundedCostTarjanSearch::initialize()
{
    GlobalState istate = state_registry.get_initial_state();
    if (task_properties::is_goal_state(m_task_proxy, istate)) {
        std::cout << "Initial state satisfies goal condition!" << std::endl;
        m_solved = true;
    } else if (!evaluate(istate, m_expansion_evaluator) 
            || !expand(istate)) {
        std::cout << "Initial state is dead-end!" << std::endl;
    }
}

bool BoundedCostTarjanSearch::evaluate(const GlobalState& state, 
                                           Evaluator* eval)
{
    if (eval == NULL) {
        m_cached_h = 0;
        return true;
    }
    EvaluationContext ctxt(state);
    EvaluationResult res = eval->compute_result(ctxt);
    if (res.is_infinite()) {
        return false;
    }
    m_cached_h = res.get_evaluator_value();
    return true;
}

bool
BoundedCostTarjanSearch::expand(const GlobalState& state)
{
    return expand(state, NULL);
}

bool
BoundedCostTarjanSearch::expand(const GlobalState& state,
                                PerLayerData* layer)
{
    static std::vector<OperatorID> aops;

    int& state_bound = m_bounds[state];

    assert(state_bound != INF && m_current_g + state_bound < bound);

    // TODO check if already computed after last refinement, and if so skip
    // evaluation
    if (!evaluate(state, m_pruning_evaluator)) {
        state_bound = INF;
        return false;
    }
    
    if (m_cached_h > state_bound) {
        state_bound = m_cached_h;
        if (m_current_g + state_bound >= bound) {
            return false;
        }
    }

    statistics.inc_expanded();
    
    bool has_zero_cost = layer != NULL;
    m_call_stack.emplace_back(state, has_zero_cost, m_neighbors.size());
    Locals& locals = m_call_stack.back();

    g_successor_generator->generate_applicable_ops(state, aops);
    statistics.inc_generated(aops.size());
    for (unsigned i = 0; i < aops.size(); i++) {
        auto op = m_task_proxy.get_operators()[aops[i]];
        GlobalState succ = state_registry.get_successor_state(
                state,
                op);
        if (evaluate(succ, m_expansion_evaluator)) {
            has_zero_cost = has_zero_cost || op.get_cost() == 0;
            locals.open[m_cached_h].emplace_back(aops[i], succ.get_id());
        } else {
            m_neighbors.emplace_back(
                    m_task->get_operator_cost(aops[i].get_index(), false),
                    succ);
        }
    }
    aops.clear();

    if (has_zero_cost && layer == NULL) {
        m_layers.emplace_back();
        m_last_layer = layer = &m_layers.back();
        locals.zero_layer = true;
    }

    if (layer != NULL) {
        ExpansionInfo& state_info = layer->state_infos[state.get_id()];
        state_info.index = state_info.lowlink = layer->index++;
        layer->stack.push_front(state);
    }

    return true;
}

SearchStatus
BoundedCostTarjanSearch::step()
{
    static std::vector<std::pair<int, GlobalState> > component_neighbors;
    static std::unordered_map<StateID, int> hashed_neighbors;

    if (m_solved) {
        Plan plan;
        while (!m_call_stack.empty()) {
            plan.push_back(m_call_stack.back().successor_op);
            m_call_stack.pop_back();
        }
        std::reverse(plan.begin(), plan.end());
        set_plan(plan);
        return SearchStatus::SOLVED;
    }

    if (m_call_stack.empty()) {
        std::cout << "Completely explored state space" << std::endl;
        return SearchStatus::FAILED;
    }

    Locals& locals = m_call_stack.back();
    ExpansionInfo* state_info = NULL;
    if (locals.zero_layer) {
        state_info = &m_last_layer->state_infos[locals.state.get_id()];
        assert(state_info->index < INF && state_info->lowlink < INF);
    }

    if (locals.successor_op != OperatorID::no_operator) {
        assert(m_last_state != StateID::no_state);
        if (m_last_state_lowlink != INF) {
            assert(state_info != NULL);
            assert(m_task->get_operator_cost(locals.successor_op.get_index(), false) == 0);
            state_info->lowlink = std::min(state_info->lowlink, m_last_state_lowlink);
            // check if a) sth has been learned and b) want to reevaluate h
        } else {
            int cost = m_task->get_operator_cost(locals.successor_op.get_index(), false);
            m_current_g -= cost;
            if (c_compute_neighbors) {
                m_neighbors.emplace_back(cost, state_registry.lookup_state(m_last_state));
            }
        }
    }

    bool all_children_explored = true;
    while (!locals.open.empty()) {
        auto it = locals.open.begin();
        auto succ = it->second.back();
        it->second.pop_back();
        if (it->second.empty()) {
            locals.open.erase(it);
        }
        locals.successor_op = succ.first;
        GlobalState succ_state = state_registry.lookup_state(succ.second);
        int cost = m_task->get_operator_cost(locals.successor_op.get_index(), false);
        m_current_g += cost;
        if (m_current_g < bound) {
            assert(cost > 0 || locals.zero_layer);
            int& succ_bound = m_bounds[succ_state];
            if (succ_bound == -1) {
                succ_bound = 0;
                if (task_properties::is_goal_state(m_task_proxy, succ_state)) {
                    m_solved = true;
                    return SearchStatus::IN_PROGRESS;
                }
            }
            bool dead = false;
            if (succ_bound != INF && m_current_g + succ_bound < bound) {
                if (cost == 0) {
                    ExpansionInfo& succ_info = m_last_layer->state_infos[succ.second];
                    if (succ_info.index == INF) {
                        if (expand(succ_state, m_last_layer)) {
                            all_children_explored = false;
                            break;
                        } else {
                            dead = true;
                            m_last_layer->state_infos.remove(succ.second);
                            assert(succ_bound == INF || m_current_g + succ_bound >= bound);
                        }
                    } else {
                        // onstack
                        state_info->lowlink = std::min(state_info->lowlink,
                                                       succ_info.lowlink);
                    }
                } else {
                    if (expand(succ_state, NULL)) {
                        all_children_explored = false;
                        break;
                    } else {
                        dead = true;
                    }
                }
            } else {
                dead = true;
            }
            if (dead && c_compute_neighbors) {
                m_neighbors.emplace_back(cost, succ_state);
            }
        } else {
            // neighbor should not be required
            // m_neighbors.emplace_back(cost, succ_state);
        }
        m_current_g -= cost;
    }

    if (all_children_explored) {
        if (state_info == NULL || state_info->index == state_info->lowlink) {
            std::unique_ptr<SuccessorComponent> neighbors = nullptr;
            if (c_refinement_toggle) {
                if (c_compute_neighbors && c_make_neighbors_unique) {
                    assert(component_neighbors.empty() && hashed_neighbors.empty());
                    component_neighbors.reserve(m_neighbors.size() - locals.neighbors_size);
                    hashed_neighbors.reserve(m_neighbors.size() - locals.neighbors_size);
                    auto it = m_neighbors.rbegin();
                    for (unsigned size = m_neighbors.size(); size > locals.neighbors_size; size--) {
                        assert(it != m_neighbors.rend());
                        auto insrted = hashed_neighbors.insert(
                                std::pair<StateID, int>(
                                    it->second.get_id(), 
                                    component_neighbors.size()));
                        if (insrted.second) {
                            component_neighbors.push_back(*it);
                        } else {
                            component_neighbors[insrted.first->second].first =
                                std::min(component_neighbors[insrted.first->second].first,
                                            it->first);
                        }
                        it++;
                    }
                    hashed_neighbors.clear();
                    neighbors = std::unique_ptr<SuccessorComponent>(
                                new SuccessorComponentIterator<std::vector<std::pair<int, GlobalState> >::iterator>(
                                    component_neighbors.begin(),
                                    component_neighbors.end()));
                } else {
                    neighbors = std::unique_ptr<SuccessorComponent>(
                            new SuccessorComponentIterator<std::deque<std::pair<int, GlobalState> >::iterator>(
                                m_neighbors.begin() + locals.neighbors_size,
                                m_neighbors.end()));
                }
            }
            if (state_info == NULL) {
                m_bounds[locals.state] = bound - m_current_g;
                if (c_refinement_toggle) {
                    SingletonComponent<GlobalState> component(locals.state);
                    c_refinement_toggle = 
                        m_refiner->notify(
                                bound - m_current_g,
                                component,
                                *neighbors);
                }
            } else {
                assert(m_last_layer != NULL);
                auto component_end = m_last_layer->stack.begin();
                while (true) {
                    m_bounds[*component_end] = bound - m_current_g;
                    m_last_layer->state_infos.remove(component_end->get_id());
                    if ((component_end++)->get_id().hash() == locals.state.get_id().hash()) {
                        break;
                    }
                }
                if (c_refinement_toggle) {
                    StateComponentIterator<std::deque<GlobalState>::iterator> component(
                            m_last_layer->stack.begin(),
                            component_end);
                    c_refinement_toggle = 
                        m_refiner->notify(
                                bound - m_current_g,
                                component,
                                *neighbors);
                }
                m_last_layer->stack.erase(m_last_layer->stack.begin(), component_end);
                if (m_last_layer->stack.empty()) {
                    // layer completely explored
                    m_layers.pop_back();
                    m_last_layer = m_layers.empty() ? NULL : &m_layers.back();
                }
            }
            component_neighbors.clear();
// #if DEBUG_BOUNDED_COST_DFS_ASSERT_NEIGHBORS
//                 for (const auto& succ : locals.successors) {
//                     bool dead = !evaluate(succ.second, m_pruning_evaluator);
//                     // std::cout << "successor " << succ.second.get_id()
//                     //     << " -> cost=" << succ.first << " dead=" << dead << " h=" << m_cached_h << " bound=" << m_bounds[succ.second] << std::endl;
//                     assert(m_bounds[succ.second] + m_current_g + succ.first >= bound);
//                     assert(dead || m_bounds[succ.second] <= m_cached_h);
//                 }
// #endif
// #if DEBUG_BOUNDED_COST_DFS_ASSERT_LEARNING
//                 if (c_refinement_toggle) {
//                     bool dead = !evaluate(locals.state, m_pruning_evaluator);
//                     // std::cout << "refinement result => dead=" << dead << " h=" << m_cached_h << std::endl;
//                     assert(dead || m_current_g + m_cached_h >= bound);
//                 }
// #endif
            if (locals.neighbors_size != m_neighbors.size()) {
                m_neighbors.erase(m_neighbors.begin() + locals.neighbors_size, m_neighbors.end());
            }
            assert(m_neighbors.size() == locals.neighbors_size);
            m_last_state_lowlink = INF;
        } else {
            m_last_state_lowlink = state_info->lowlink;
        }
        m_last_state = locals.state.get_id();
        m_call_stack.pop_back();
    }

    return SearchStatus::IN_PROGRESS;
}

void BoundedCostTarjanSearch::print_statistics() const
{
    SearchEngine::print_statistics();
    std::cout << "Registered: " << state_registry.size() << " state(s)" << std::endl;
    statistics.print_detailed_statistics();
    
#ifndef NDEBUG
    hc_heuristic::HCHeuristic* h = dynamic_cast<hc_heuristic::HCHeuristic*>(m_pruning_evaluator);
    if (h != NULL) {
        h->dump_conjunctions();
    }
#endif
}

void
BoundedCostTarjanSearch::add_options_to_parser(options::OptionParser& parser)
{
    parser.add_option<Evaluator *>("eval");
    parser.add_option<std::shared_ptr<HeuristicRefiner> >("learn", "", options::OptionParser::NONE);
    parser.add_option<bool>("unique_neighbors", "", "false");
    add_cost_type_option_to_parser(parser);
    SearchEngine::add_options_to_parser(parser);
}

}
}

static
std::shared_ptr<SearchEngine> _parse(options::OptionParser& p) {
    conflict_driven_learning::bounded_cost::BoundedCostTarjanSearch::add_options_to_parser(p);
    options::Options opts = p.parse();
    if (!p.dry_run()) {
        return std::make_shared<conflict_driven_learning::bounded_cost::BoundedCostTarjanSearch>(opts);
    }
    return nullptr;
}

static PluginShared<SearchEngine> _plugin("bounded_cost_dfsz", _parse);

