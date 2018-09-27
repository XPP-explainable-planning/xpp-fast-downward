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

#include <limits>
#include <algorithm>

namespace conflict_driven_learning {
namespace bounded_cost {

static const int INF = std::numeric_limits<int>::infinity();

BoundedCostDepthFirstSearch::Locals::Locals(const GlobalState& state)
    : state(state), successor_op(OperatorID::no_operator)
{}

BoundedCostDepthFirstSearch::BoundedCostDepthFirstSearch(const options::Options& opts)
    : SearchEngine(opts)
    , c_refinement_toggle(false)
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
    for (int op = 0; op < task->get_num_operators(); op++) {
        if (task->get_operator_cost(op, false) == 0) {
            std::cerr << "bounded cost depth first search does not support 0-cost actions"
                        << std::endl;
            utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
        }
    }
    m_current_g = 0;
}

void
BoundedCostDepthFirstSearch::initialize()
{
    GlobalState istate = state_registry.get_initial_state();
    if (task_properties::is_goal_state(task_proxy, istate)) {
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

    if (state_bound == INF || !evaluate(state, m_pruning_evaluator)) {
        state_bound = INF;
        return false;
    }

    state_bound = std::max(state_bound, m_cached_h);
    if (m_current_g + state_bound >= bound) {
        return false;
    }

    statistics.inc_expanded();
    m_call_stack.emplace_back(state);
    Locals& locals = m_call_stack.back();

    g_successor_generator->generate_applicable_ops(state, aops);
    statistics.inc_generated(aops.size());
    for (unsigned i = 0; i < aops.size(); i++) {
        GlobalState succ = state_registry.get_successor_state(
                state,
                task_proxy.get_operators()[aops[i]]);
        if (evaluate(succ, m_expansion_evaluator)) {
            locals.open[m_cached_h].emplace_back(aops[i], succ.get_id());
        } else {
            locals.successors.push_back(succ);
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
        m_current_g -= task->get_operator_cost(locals.successor_op.get_index(), false);
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
        GlobalState succ_state = state_registry.lookup_state(succ.second);
        if (task_properties::is_goal_state(task_proxy, succ_state)) {
            m_solved = true;
            return SearchStatus::IN_PROGRESS;
        }
        locals.successors.push_back(succ_state);
        int cost = task->get_operator_cost(succ.first.get_index(), false);
        m_current_g += cost;
        if (expand(succ_state)) {
            expanded = false;
            break;    
        }
        m_current_g -= cost;
    }

    if (expanded) {
        if (c_refinement_toggle) {
            c_refinement_toggle = 
                m_refiner->notify(bound - m_current_g,
                                SingleStateComponent(locals.state),
                                StateComponentIterator<std::vector<GlobalState>::iterator>(
                                    locals.successors.begin(),
                                    locals.successors.end()));
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
}

void
BoundedCostDepthFirstSearch::add_options_to_parser(options::OptionParser& parser)
{
    parser.add_option<Evaluator *>("eval");
    parser.add_option<std::shared_ptr<HeuristicRefiner> >("learn", "", options::OptionParser::NONE);
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

