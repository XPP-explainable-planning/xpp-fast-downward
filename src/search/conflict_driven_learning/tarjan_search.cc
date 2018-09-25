
#include "tarjan_search.h"

#include "state_component.h"

#include "../globals.h"
#include "../global_state.h"
#include "../state_registry.h"
#include "../task_proxy.h"
#include "../evaluation_context.h"
#include "../evaluation_result.h"
#include "../task_utils/successor_generator.h"
#include "../task_utils/task_properties.h"

#include "../option_parser.h"
#include "../plugin.h"

#include <algorithm>
#include <iostream>
#include <set>

#ifndef NDEBUG
#define NDEBUG_VERIFY_RECOGNIZED_NEIGHBORS 1
#define NDEBUG_VERIFY_REFINEMENT 1
#endif

struct GlobalStateLess {
  bool operator()(const GlobalState& s1, const GlobalState& s2) const {
      return s1.get_id().hash() < s2.get_id().hash();
  }
};

using StateSet = std::set<GlobalState, GlobalStateLess>;

namespace conflict_driven_learning
{
namespace tarjan_search
{

TarjanSearch::CallStackElement::CallStackElement(
    SearchNode &node) :
    node(node), last_successor_id(StateID::no_state), succ_result(DFSResult::FAILED)
{}

TarjanSearch::TarjanSearch(const options::Options &opts)
    : SearchEngine(opts),
      c_recompute_u(opts.get<bool>("recompute_u")),
      c_refine_initial_state(opts.get<bool>("refine_initial_state")),
      c_dead_end_refinement(opts.contains("learn")),
      c_compute_recognized_neighbors(c_dead_end_refinement),
      m_cached_h_value(0),
      m_guidance(opts.get<Evaluator *>("eval")),
      m_learner(opts.contains("learn") ? opts.get<std::shared_ptr<ConflictLearner>>("learn") : nullptr),
      m_search_space(&state_registry),
      m_current_index(0),
      m_result(DFSResult::FAILED),
      m_open_states(1)
{
    c_compute_recognized_neighbors = m_learner != nullptr && m_learner->requires_recognized_neighbors();
    m_dead_end_identifier = m_learner == nullptr ? nullptr : m_learner->get_underlying_heuristic();
}

void TarjanSearch::initialize()
{
    std::cout << "Initializing tarjan search ..." << std::endl;
    std::cout << (c_compute_recognized_neighbors ? "M" : "Not m")
              << "aintaining recognized neighbors." << std::endl;

    m_open_states = 1;
    m_current_index = 0;
    m_current_depth = 0;
    m_result = DFSResult::FAILED;
    GlobalState istate = state_registry.get_initial_state();
    SearchNode inode = m_search_space[istate];
    if (!evaluate(istate)) {
        std::cout << "Initial state is dead-end!" << std::endl;
        inode.mark_recognized_dead_end();
    } else {
        inode.open_initial_state(get_h_value());
        if (task_properties::is_goal_state(task_proxy, istate)) {
            m_result = DFSResult::SOLVED;
        } else {
            expand(istate);
        }
    }
}

bool TarjanSearch::evaluate(const GlobalState& state)
{
    statistics.inc_evaluated_states();
    statistics.inc_evaluations();
    EvaluationContext ctxt(state);
    EvaluationResult res = m_guidance->compute_result(ctxt);
    if (res.is_infinite()) {
        return false;
    }
    m_cached_h_value = res.get_evaluator_value();
    return true;
}

bool TarjanSearch::evaluate_dead_end_heuristic(const GlobalState& state)
{
    if (m_dead_end_identifier == nullptr) {
        return false;
    }
    EvaluationContext ctxt(state);
    EvaluationResult res = m_dead_end_identifier->compute_result(ctxt);
    if (res.is_infinite()) {
        return true;
    }
    return false;
}

int TarjanSearch::get_h_value() const
{
    return m_cached_h_value;
}

bool TarjanSearch::expand(const GlobalState& state)
{
    static std::vector<OperatorID> aops;

    SearchNode node = m_search_space[state];
    m_open_states--;

    if (evaluate_dead_end_heuristic(state)) {
        node.mark_recognized_dead_end();
        return false;
    }

    statistics.inc_expanded();

    node.close(m_current_index);
    m_current_index++;
    m_stack.push_front(state);
    if (c_compute_recognized_neighbors) {
        m_rn_offset.push_front(m_recognized_neighbors.size());
    }
    m_call_stack.emplace_back(node);

    m_open_list.push_layer();
    g_successor_generator->generate_applicable_ops(state, aops);
    statistics.inc_generated(aops.size());
    for (unsigned i = 0; i < aops.size(); i++) {
        GlobalState succ = state_registry.get_successor_state(
                state,
                task_proxy.get_operators()[aops[i]]);
        SearchNode succ_node = m_search_space[succ];
        assert(succ_node.is_new() || succ_node.get_lowlink() <= succ_node.get_index());
        if (succ_node.is_new()) {
            if (evaluate(succ)) {
                succ_node.open(node, aops[i], get_h_value());
                m_open_states++;
            } else {
              statistics.inc_dead_ends();
                succ_node.mark_recognized_dead_end();
            }
        }
        if (!succ_node.is_dead_end()) {
            m_open_list.push(node.get_h(), succ.get_id());
        } else if (c_compute_recognized_neighbors) {
            m_recognized_neighbors.push_back(succ.get_id());
        }
    }
    aops.clear();

    m_current_depth++;
    return true;
}

SearchStatus TarjanSearch::step()
{
    static StateSet recognized_neighbors;

    if (m_result == DFSResult::SOLVED) {
        return SearchStatus::SOLVED;
    }

    if (m_call_stack.empty()) {
        return SearchStatus::FAILED;
    }

    CallStackElement& elem = m_call_stack.back();

    bool in_dead_end_component = false;
    // if backtracked
    if (elem.last_successor_id != StateID::no_state) {
        GlobalState state = state_registry.lookup_state(elem.node.get_state_id());
        GlobalState succ = state_registry.lookup_state(elem.last_successor_id);
        SearchNode succ_node = m_search_space[succ];
        assert(succ_node.is_closed());
        elem.node.update_lowlink(succ_node.get_lowlink());
        elem.succ_result = std::max(elem.succ_result, m_result);
        if (succ_node.is_dead_end()) {
            if (m_result == DFSResult::DEAD_END_COMPONENT
                || (m_result == DFSResult::SCC_COMPLETED
                    && c_recompute_u
                    && evaluate_dead_end_heuristic(state))) {
                in_dead_end_component = true;
                elem.node.mark_recognized_dead_end();
                while (m_open_list.layer_size() > 0) {
                    StateID state_id = m_open_list.pop_minimum();
                    SearchNode node =
                        m_search_space[state_registry.lookup_state(state_id)];
                    if (!node.is_closed()) {
                        node.mark_recognized_dead_end();
                        statistics.inc_dead_ends();
                    } else if (node.is_onstack()) {
                        elem.node.update_lowlink(node.get_index());
                    } else {
                        assert(node.is_dead_end());
                        node.mark_recognized_dead_end();
                    }
                }
            } else if (c_compute_recognized_neighbors) {
                m_recognized_neighbors.push_back(succ.get_id());
            }
        }
    }

    bool fully_expanded = true;
    while (m_open_list.layer_size() > 0) {
        StateID succ_id = m_open_list.pop_minimum();
        GlobalState succ = state_registry.lookup_state(succ_id);
        SearchNode succ_node = m_search_space[succ];

        if (succ_node.is_dead_end()) {
            if (c_compute_recognized_neighbors) {
                m_recognized_neighbors.push_back(succ_id);
            }
        } else if (!succ_node.is_closed()) {
            if (task_properties::is_goal_state(task_proxy, succ)) {
                std::vector<OperatorID> plan;
                m_search_space.trace_path(succ_node, plan);
                set_plan(plan);
                return SearchStatus::SOLVED;
            }
            if (expand(succ)) {
                elem.last_successor_id = succ_id;
                fully_expanded = false;
                break;
            } else if (c_compute_recognized_neighbors) {
                assert(succ_node.is_dead_end());
                m_recognized_neighbors.push_back(succ_id);
            }
        } else if (succ_node.is_onstack()) {
            elem.node.update_lowlink(succ_node.get_index());
        } else {
            assert(false);
        }
    }

    m_result = DFSResult::FAILED;

    if (fully_expanded) {
        m_result = in_dead_end_component
                   ? DFSResult::DEAD_END_COMPONENT
                   : DFSResult::FAILED;

        assert(elem.node.get_lowlink() <= elem.node.get_index());
        if (elem.node.get_index() == elem.node.get_lowlink()) {
            std::deque<GlobalState>::iterator it = m_stack.begin();
            std::deque<unsigned>::iterator rnoff_it = m_rn_offset.begin();
            while (true) {
                SearchNode snode = m_search_space[*it];
                snode.popped_from_stack();
                if (in_dead_end_component) {
                    if (!snode.is_dead_end()) {
                        /* m_progress.inc_partially_expanded_dead_ends(); */
                        snode.mark_recognized_dead_end();
                    }
                } else {
                    assert(!snode.is_dead_end());
                    snode.mark_dead_end();
                    /* m_progress.inc_expanded_dead_ends(); */
                }
                if ((it++)->get_id() == elem.node.get_state_id()) {
                    break;
                }
                if (c_compute_recognized_neighbors) {
                    rnoff_it++;
                }
            }
            bool entered_refinement = false;
            if (c_dead_end_refinement
                && !in_dead_end_component
                && (m_open_states > 0 || c_refine_initial_state)
                && (elem.succ_result != DFSResult::UNRECOGNIZED || !c_compute_recognized_neighbors)) {
                entered_refinement = true;
                /* m_progress.inc_dead_end_refinements(); */
                if (c_compute_recognized_neighbors) {
                    std::deque<StateID>::iterator rnid_it =
                        m_recognized_neighbors.begin() + *rnoff_it;
                    while (rnid_it != m_recognized_neighbors.end()) {
                        recognized_neighbors.insert(state_registry.lookup_state(*rnid_it));
                        rnid_it++;
                    }
#if NDEBUG_VERIFY_RECOGNIZED_NEIGHBORS
                    static std::vector<OperatorID> aops;
                    StateSet component;
                    std::deque<GlobalState>::iterator compit = m_stack.begin();
                    while (compit != it) {
                        component.insert(*compit);
                        compit++;
                    }
                    compit = m_stack.begin();
                    while (compit != it) {
                        g_successor_generator->generate_applicable_ops(*compit, aops);
                        for (unsigned i = 0; i < aops.size(); i++) {
                            GlobalState succ = state_registry.get_successor_state(
                                    *compit,
                                    task_proxy.get_operators()[aops[i]]);
                            if (!component.count(succ) && !recognized_neighbors.count(succ)) {
                                SearchNode succnode = m_search_space[succ];
                                std::cout << "STATE NOT FOUND! " << succ.get_id()
                                          << ": " << succnode.is_new()
                                          << "|" << succnode.is_closed()
                                          << "|" << succnode.is_dead_end()
                                          << "|" << succnode.is_onstack()
                                          << " " << succnode.get_index()
                                          << " " << succnode.get_lowlink()
                                          << std::endl;
                            }
                            assert(component.count(succ) || recognized_neighbors.count(succ));
                            assert(!recognized_neighbors.count(succ)
                                || (m_search_space[succ].is_dead_end() && evaluate_dead_end_heuristic(succ)));
                        }
                        compit++;
                        aops.clear();
                    }
#endif
                }
                c_dead_end_refinement = m_learner->notify_dead_end_component(
                        StateComponentIterator<std::deque<GlobalState>::iterator>(m_stack.begin(), it),
                        StateComponentIterator<StateSet::iterator>(recognized_neighbors.begin(), recognized_neighbors.end()));
                recognized_neighbors.clear();
                if (!c_dead_end_refinement && c_compute_recognized_neighbors) {
                    c_compute_recognized_neighbors = false;
                    m_recognized_neighbors.clear();
                    m_rn_offset.clear();
                }
                if (c_dead_end_refinement) {
                    for (auto compit = m_stack.begin(); compit != it; compit++) {
#if NDEBUG_VERIFY_REFINEMENT
                        assert(evaluate_dead_end_heuristic(*compit));
#endif
                        assert(m_search_space[*compit].is_dead_end());
                        m_search_space[*compit].mark_recognized_dead_end();
                    }
                }
            }
            m_stack.erase(m_stack.begin(), it);
            if (c_compute_recognized_neighbors) {
                m_recognized_neighbors.erase(m_recognized_neighbors.begin() + *rnoff_it,
                                             m_recognized_neighbors.end());
                m_rn_offset.erase(m_rn_offset.begin(), ++rnoff_it);
            }
            m_result = (in_dead_end_component || (entered_refinement && c_dead_end_refinement))
                ? DFSResult::SCC_COMPLETED
                : DFSResult::UNRECOGNIZED;
        }
        m_call_stack.pop_back();
        m_current_depth--;
        m_open_list.pop_layer();
    }

    return SearchStatus::IN_PROGRESS;
}

void TarjanSearch::print_statistics() const
{
    SearchEngine::print_statistics();
    std::cout << "Registered: " << state_registry.size() << " state(s)" << std::endl;
    statistics.print_detailed_statistics();
}

void TarjanSearch::add_options_to_parser(options::OptionParser &parser)
{
    SearchEngine::add_options_to_parser(parser);
    parser.add_option<Evaluator *>("eval");
    parser.add_option<std::shared_ptr<ConflictLearner>>("learn", "", options::OptionParser::NONE);
    parser.add_option<bool>("recompute_u",
                            "recompute dead-end detection heuristic after each refinement",
                            "true");
    parser.add_option<bool>("refine_initial_state", "", "false");
}

}
}

static std::shared_ptr<SearchEngine>
_parse(options::OptionParser& parser)
{
    conflict_driven_learning::tarjan_search::TarjanSearch::add_options_to_parser(parser);
    options::Options opts = parser.parse();
    if (!parser.dry_run()) {
        return std::make_shared<conflict_driven_learning::tarjan_search::TarjanSearch>(opts);
    }
    return nullptr;
}

static PluginShared<SearchEngine> _plugin("dfs", _parse);