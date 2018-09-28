#include "bounded_cost_depth_first_search.h"
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

BoundedCostDepthFirstSearch::Locals::Locals(const GlobalState& state)
    : state(state), successor_op(OperatorID::no_operator)
{}

BoundedCostDepthFirstSearch::BoundedCostDepthFirstSearch(const options::Options& opts)
    : SearchEngine(opts)
    , c_refinement_toggle(false)
    , m_task(OperatorCost(opts.get_enum("cost_type")) != OperatorCost::NORMAL ? std::make_shared<tasks::CostAdaptedTask>(opts) : task)
    , m_task_proxy(*m_task)
    , m_expansion_evaluator(opts.get<Evaluator *>("eval"))
    , m_pruning_evaluator(NULL)
    , m_refiner(opts.contains("learn") ? opts.get<std::shared_ptr<HeuristicRefiner> >("learn") : nullptr)
    , m_bounds(-1)
    , m_solved(false)
{
    if (m_refiner != nullptr) {
        c_refinement_toggle = true;
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
BoundedCostDepthFirstSearch::initialize()
{
    GlobalState istate = state_registry.get_initial_state();
    m_bounds[istate] = 0;
    if (task_properties::is_goal_state(m_task_proxy, istate)) {
        std::cout << "Initial state satisfies goal condition!" << std::endl;
        m_solved = true;
    } else if (!evaluate(istate, m_expansion_evaluator) 
            || !expand(istate)) {
        std::cout << "Initial state is dead-end!" << std::endl;
    }
    m_current_g = 0;
}

bool BoundedCostDepthFirstSearch::evaluate(const GlobalState& state, 
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
BoundedCostDepthFirstSearch::expand(const GlobalState& state)
{
    static std::vector<OperatorID> aops;

    int& state_bound = m_bounds[state];
    assert(state_bound >= 0);

    if (m_current_g + state_bound >= bound) {
        return false;
    }

    if (state_bound == INF || !evaluate(state, m_pruning_evaluator)) {
        std::cout << state_bound << ": " <<  (state_bound == INF) << std::endl;
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
    m_call_stack.emplace_back(state);
    Locals& locals = m_call_stack.back();

    g_successor_generator->generate_applicable_ops(state, aops);
    statistics.inc_generated(aops.size());
    for (unsigned i = 0; i < aops.size(); i++) {
        GlobalState succ = state_registry.get_successor_state(
                state,
                m_task_proxy.get_operators()[aops[i]]);
        if (evaluate(succ, m_expansion_evaluator)) {
            locals.open[m_cached_h].emplace_back(aops[i], succ.get_id());
        } else {
            locals.successors.emplace_back(
                    m_task->get_operator_cost(aops[i].get_index(), false),
                    succ);
        }
    }
    aops.clear();

    return true;
}

SearchStatus
BoundedCostDepthFirstSearch::step()
{
    if (m_solved) {
        if (!m_call_stack.empty()) {
            m_plan.push_back(m_call_stack.back().successor_op);
            m_call_stack.pop_back();
            return SearchStatus::IN_PROGRESS;
        } else {
            std::reverse(m_plan.begin(), m_plan.end());
            set_plan(m_plan);
            return SearchStatus::SOLVED;
        }
    }

    if (m_call_stack.empty()) {
        std::cout << "Completely explored state space" << std::endl;
        return SearchStatus::FAILED;
    }

    Locals& locals = m_call_stack.back();

    if (locals.successor_op != OperatorID::no_operator) {
        m_current_g -= m_task->get_operator_cost(locals.successor_op.get_index(), false);
    }

    bool expanded = true;
    while (!locals.open.empty()) {
        auto it = locals.open.begin();
        auto succ = it->second.back();
        it->second.pop_back();
        if (it->second.empty()) {
            locals.open.erase(it);
        }
        locals.successor_op = succ.first;
        int cost = m_task->get_operator_cost(locals.successor_op.get_index(), false);
        m_current_g += cost;
        if (m_current_g < bound) {
            GlobalState succ_state = state_registry.lookup_state(succ.second);
            int& succ_bound = m_bounds[succ_state];
            if (succ_bound == -1) {
                succ_bound = 0;
                if (task_properties::is_goal_state(m_task_proxy, succ_state)) {
                    m_solved = true;
                    return SearchStatus::IN_PROGRESS;
                }
            }
            locals.successors.emplace_back(cost, succ_state);
            if (expand(succ_state)) {
                expanded = false;
                break;
            }
        }
        m_current_g -= cost;
    }

    if (expanded) {
        if (c_refinement_toggle) {
#if DEBUG_BOUNDED_COST_DFS_ASSERT_NEIGHBORS
            for (const auto& succ : locals.successors) {
                bool dead = !evaluate(succ.second, m_pruning_evaluator);
                // std::cout << "successor " << succ.second.get_id()
                //     << " -> cost=" << succ.first << " dead=" << dead << " h=" << m_cached_h << " bound=" << m_bounds[succ.second] << std::endl;
                assert(dead || m_bounds[succ.second] <= m_cached_h);
                assert(m_bounds[succ.second] == INF ||
                        (m_bounds[succ.second] + m_current_g + succ.first) >= bound);
            }
#endif
            c_refinement_toggle = 
                m_refiner->notify(
                        bound - m_current_g,
                        SingletonComponent<GlobalState>(locals.state),
                        SuccessorComponentIterator<std::vector<std::pair<int, GlobalState> >::iterator>(
                            locals.successors.begin(), locals.successors.end()));
#if DEBUG_BOUNDED_COST_DFS_ASSERT_LEARNING
            if (c_refinement_toggle) {
                bool dead = !evaluate(locals.state, m_pruning_evaluator);
                // std::cout << "refinement result => dead=" << dead << " h=" << m_cached_h << std::endl;
                assert(dead || m_current_g + m_cached_h >= bound);
            }
#endif
        }
        assert(m_bounds[locals.state] < bound - m_current_g);
        m_bounds[locals.state] = bound - m_current_g;
        m_call_stack.pop_back();
    }

    return SearchStatus::IN_PROGRESS;
}

void BoundedCostDepthFirstSearch::print_statistics() const
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
BoundedCostDepthFirstSearch::add_options_to_parser(options::OptionParser& parser)
{
    parser.add_option<Evaluator *>("eval");
    parser.add_option<std::shared_ptr<HeuristicRefiner> >("learn", "", options::OptionParser::NONE);
    add_cost_type_option_to_parser(parser);
    SearchEngine::add_options_to_parser(parser);
}

}
}

static
std::shared_ptr<SearchEngine> _parse(options::OptionParser& p) {
    conflict_driven_learning::bounded_cost::BoundedCostDepthFirstSearch::add_options_to_parser(p);
    options::Options opts = p.parse();
    if (!p.dry_run()) {
        return std::make_shared<conflict_driven_learning::bounded_cost::BoundedCostDepthFirstSearch>(opts);
    }
    return nullptr;
}

static PluginShared<SearchEngine> _plugin("bounded_cost_dfs", _parse);

