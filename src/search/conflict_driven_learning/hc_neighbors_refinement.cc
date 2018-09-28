#include "hc_neighbors_refinement.h"
#include "state_component.h"
#include "strips_compilation.h"
#include "set_utils.h"

#include "../global_state.h"
#include "../tasks/root_task.h"

// #include "../../globals.h"
// #include "../../operator.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../utils/timer.h"

#include <algorithm>
#include <iostream>
#include <cstdio>
#include <limits>

#ifndef NDEBUG
#define DEBUG_VERBOSE_PRINT_OUTS 0
#endif

// static std::ostream &operator<<(std::ostream &out, const std::vector<unsigned> &v)
// {
//     out << "[";
//     for (unsigned i = 0; i < v.size(); i++) {
//         if (i > 0) {
//             out << ", ";
//         }
//         out << v[i];
//     }
//     out << "]";
//     return out;
// }

namespace conflict_driven_learning
{
namespace hc_heuristic
{
    
static const int INF = std::numeric_limits<int>::max();

constexpr const unsigned HCNeighborsRefinement::UNASSIGNED = -1;

HCNeighborsRefinement::HCNeighborsRefinement(const Options &opts)
    : HCHeuristicRefiner(opts)
    // , m_task(tasks::g_root_task)
{
    m_num_refinements = 0;
}

int HCNeighborsRefinement::get_conjunction_value(unsigned id)
{
    const auto& data = m_hc->get_conjunction_data(id);
    return data.achieved() ? data.cost : INF;
}

void HCNeighborsRefinement::initialize()
{
    HCHeuristicRefiner::initialize();
    m_strips_task = &strips::get_task();
}

bool HCNeighborsRefinement::refine_heuristic(
        int bound,
    StateComponent &component,
    SuccessorComponent& neighbors)
{
    std::shared_ptr<AbstractTask> m_task = tasks::g_root_task;

    m_hc->set_early_termination_and_nogoods(false);

    m_num_conjunctions_before_refinement = m_hc->num_conjunctions();

    m_current_state_cost.clear();
    m_fact_to_negated_component.clear();
    m_negated_component_to_facts.clear();
    m_conjunction_to_successors.clear();
    m_successor_to_conjunctions.clear();

    m_chosen.clear();

    // PartialState root_state(g_variable_domain.size());
    bool terminate = false;
    bool result = false;
    m_component_size = 0;
    m_current_state_cost.resize(m_hc->num_conjunctions(), 0);
    m_fact_to_negated_component.resize(strips::num_facts());
    while (!component.end()) {
        const GlobalState &state = component.current();
        if (m_component_size == 0) {
#if DEBUG_VERBOSE_PRINT_OUTS
            std::cout << "refine(" << state.get_id() << "[";
            for (int var = 0; var < m_task->get_num_variables(); var++) {
                std::cout 
                    << (var > 0 ? ", " : "")
                    << m_task->get_fact_name(FactPair(var, state[var]));
            }
            std::cout << ", bound=" << bound << ")" << std::endl;
#endif
            int res = m_hc->evaluate(state);
            if (res == HCHeuristic::DEAD_END || res >= bound) {
                terminate = true;
                result = true;
                break;
            }
            for (unsigned i = 0; i < m_hc->num_conjunctions(); i++) {
                m_current_state_cost[i] = m_hc->get_conjunction_data(i).cost;
            }
        }
        m_negated_component_to_facts.emplace_back();
        unsigned p = 0;
        for (int var = 0; var < m_task->get_num_variables(); var++) {
            for (int val = 0; val < m_task->get_variable_domain_size(var); val++) {
                if (state[var] != val) {
                    assert(p == strips::get_fact_id(var, val));
                    m_fact_to_negated_component[p].push_back(m_component_size);
                    m_negated_component_to_facts[m_component_size].push_back(p);
                }
                p++;
            }
        }
        component.next();
        ++m_component_size;
    }

    if (!terminate) {
        m_num_successors = 0;
        m_conjunction_to_successors.resize(m_hc->num_conjunctions());
        while (!neighbors.end()) {
            const auto& succ = neighbors.current();
#ifndef NDEBUG
            int res =
#endif
            m_hc->evaluate(succ.second);
            assert(res == HCHeuristic::DEAD_END || res + succ.first >= bound);
            m_successor_to_conjunctions.emplace_back();
            for (unsigned cid = 0; cid < m_conjunction_to_successors.size(); cid++) {
                int value = get_conjunction_value(cid);
                if (value != INF) {
                    value += succ.first;
                }
                m_conjunction_to_successors[cid][value].push_back(m_num_successors);
                m_successor_to_conjunctions[m_num_successors][value].push_back(cid);
            }
            m_num_successors++;
            neighbors.next();
        }

        m_num_refinements++;

        for (unsigned x = 0; x < m_hc->num_counters(); x++) {
            m_hc->get_counter(x).unsat = 0;
        }
        for (unsigned cid = 0; cid < m_hc->num_conjunctions(); cid++) {
            ConjunctionData &data = m_hc->get_conjunction_data(cid);
            data.cost = m_current_state_cost[cid] == INF ? ConjunctionData::UNACHIEVED : m_current_state_cost[cid]; 
            if (!data.achieved()) {
                for (Counter *c : data.pre_of) {
                    c->unsat++;
                }
            }
        }

        m_chosen.resize(std::max(m_component_size, m_num_successors), UNASSIGNED);
        m_num_covered_by.resize(m_hc->num_conjunctions());

        push_conflict_for(m_strips_task->get_goal(), bound);
        while (!m_open.empty() && !size_limit_reached()) {
            OpenElement &elem = m_open.back();
            if (elem.i == elem.achievers.size()) {
                m_open.pop_back();
            } else {
                unsigned counter = elem.achievers[elem.i++];
                assert(counter < m_hc->num_counters());
                if (m_hc->get_counter(counter).unsat == 0) {
                    unsigned op = m_hc->get_action_id(counter);
                    const strips::Action &action =
                        m_strips_task->get_action(op);
                    if (elem.bound - action.cost >= 0) {
                        assert(set_utils::intersects(elem.conj, action.add));
                        assert(!set_utils::intersects(elem.conj, action.del));
                        std::set_union(elem.conj.begin(), elem.conj.end(),
                                    action.pre.begin(), action.pre.end(),
                                    std::back_inserter(m_regression));
                        set_utils::inplace_difference(m_regression, action.add);
                        assert(!m_strips_task->contains_mutex(m_regression));
#if 0
                        std::cout << "<regression of conflict=";
                        print_conjunction(m_regression, false);
                        std::cout << " through action "
                                << g_operators[op].get_name() << std::endl
                                << "<regression = ";
                        print_conjunction(m_regression, true);
#endif
                        push_conflict_for(m_regression, elem.bound - action.cost);
                        m_regression.clear();
                    }
                }
            }
        }
        result = m_open.empty();
        m_open.clear();
    }

    m_hc->set_early_termination_and_nogoods(true);

//#ifndef NDEBUG
//    if (result) {
//
//    std::cout << "goal: ";
//    for (auto i : m_strips_task->get_goal()) {
//        std::cout << " " << i;
//    }
//    std::cout << std::endl;
//
//    std::vector<unsigned> ids;
//    m_hc->get_satisfied_conjunctions(strips::get_task().get_goal(),
//                                     ids);
//    for (unsigned id : ids) {
//        for (unsigned g : m_hc->get_conjunction(id)) {
//            std::cout << " " << g;
//        }
//        std::cout << std::endl;
//        assert(std::count(m_hc->get_conjunction_data(id).pre_of.begin(),
//                          m_hc->get_conjunction_data(id).pre_of.end(),
//                          &m_hc->get_goal_counter()));
//    }
//    assert(m_hc->get_goal_counter().preconditions == ids.size());
//
//    std::cout << "component = " << std::endl;
//    component.reset();
//    while (!component.end()) {
//        std::cout << "  " << component.current().get_id() << ":";
//        for (int var = 0; var < m_task->get_num_variables(); var++) {
//            std::cout << " " << strips::get_fact_id(var, component.current()[var]);
//        }
//        std::cout << std::endl;
//        component.next();
//    }
//
//
//        std::cout << "test =[" << std::flush;
//        component.reset();
//        while (!component.end()) {
//            int res = m_hc->evaluate(component.current());
//            if (res != HCHeuristic::DEAD_END){
//                std::cout << "(" << res << "!=inf)" << std::flush;
//            }
//            std::cout << " " << component.current().get_id() << std::flush;
//            assert(res == HCHeuristic::DEAD_END);
//            component.next();
//        }
//        std::cout << " ] passed -> "
//            << (m_hc.get())<< std::endl;
//    }
//#endif

    return result;
}

int HCNeighborsRefinement::get_cost(const std::vector<unsigned>& subgoal)
{
    int res = 0;
    m_hc->get_satisfied_conjunctions(subgoal, [this, &res](const unsigned& id) {
        res = std::max(res, m_current_state_cost[id]);
    });
    return res; // std::min(0, res);
}

void HCNeighborsRefinement::push_conflict_for(
    const std::vector<unsigned> &subgoal,
    int bound)
{
#if DEBUG_VERBOSE_PRINT_OUTS
    std::cout << "push_conflict_for(";
    m_hc->dump_conjunction(subgoal);
    std::cout << ", bound=" << bound << ")" << std::endl;
#endif
    if (get_cost(subgoal) >= bound) {
        return;
    }
    compute_conflict(subgoal, m_conflict, bound);
#if DEBUG_VERBOSE_PRINT_OUTS
    std::cout << "  -> conflict=";
    m_hc->dump_conjunction(m_conflict);
    std::cout << std::endl;
#endif
    std::pair<unsigned, bool> res =
        m_hc->insert_conjunction_and_update_data_structures(m_conflict, bound);
    m_conflict.clear();
    if (res.second) {
        m_current_state_cost.push_back(bound);
    }
    // m_hc->mark_unachieved(res.first);
    m_open.emplace_back(m_hc->get_conjunction(res.first),
                        m_hc->get_conjunction_achievers(res.first),
                        bound);
}

void HCNeighborsRefinement::get_previously_statisfied_conjunctions(
    const std::vector<unsigned> &subgoal,
    std::vector<unsigned> &satisfied)
{
    m_hc->get_satisfied_conjunctions(subgoal,
    [this, &satisfied](const unsigned & cid) {
        if (cid < m_num_conjunctions_before_refinement) {
            satisfied.push_back(cid);
        }
    });
}

// bool HCNeighborsRefinement::is_subgoal_covered_by_all_successors(
//     const std::vector<unsigned> &subgoal)
// {
//     std::vector<bool> covered(m_num_successors, false);
//     unsigned hit = 0;
//     m_hc->get_satisfied_conjunctions(subgoal,
//     [this, &covered, &hit](const unsigned & cid) {
//         for (unsigned i = 0; i < m_num_successors; i++) {
//             if (!covered[i] && set_utils::contains(m_successor_to_conjunctions[i], cid)) {
//                 covered[i] = true;
//                 hit++;
//             }
//         }
//     });
//     return hit == m_num_successors;
// }
// 
// bool HCNeighborsRefinement::is_contained_in_component(
//     const std::vector<unsigned> &conflict)
// {
//     for (unsigned i = 0; i < m_component_size; i++) {
//         if (!set_utils::intersects(conflict,
//                                    m_negated_component_to_facts[i])) {
//             return true;
//         }
//     }
//     return false;
// }

unsigned HCNeighborsRefinement::collect_greedy_minimal(
    const std::vector<unsigned> &superset,
    std::vector<unsigned> &num_covered,
    const std::vector<std::map<int, std::vector<unsigned> > > &covers,
    const std::vector<std::map<int, std::vector<unsigned> > > &covered_by,
    std::vector<unsigned> &chosen,
    int bound)
{
    unsigned left = covered_by.size();
    while (left > 0) {
        unsigned best_size = 0;
        unsigned best_coverage = 0;
        unsigned best_cid = UNASSIGNED;
        for (const unsigned &elem : superset) {
            if (num_covered[elem] > best_coverage
                || (num_covered[elem] == best_coverage
                    && m_hc->get_conjunction_size(elem) < best_size)) {
                best_size = m_hc->get_conjunction_size(elem);
                best_coverage = num_covered[elem];
                best_cid = elem;
            }
        }
        assert(best_cid == UNASSIGNED || best_coverage > 0);
        if (best_cid == UNASSIGNED) {
            break;
        }
        for (auto it = covers[best_cid].lower_bound(bound); it != covers[best_cid].end(); it++) {
            for (const auto& succ : it->second) {
                if (chosen[succ] == UNASSIGNED) {
                    left--;
                    for (auto it2 = covered_by[succ].lower_bound(bound);
                            it2 != covered_by[succ].end();
                            it2++) {
                        for (const unsigned &elem : it2->second) {
                            num_covered[elem]--;
                        }
                    }
                }
                chosen[succ] = best_cid;
            }
        }
        assert(num_covered[best_cid] == 0);
    }
    return left;
}

unsigned HCNeighborsRefinement::collect_greedy_minimal(
    const std::vector<unsigned> &superset,
    std::vector<unsigned> &num_covered,
    const std::vector<std::vector<unsigned> > &covers,
    const std::vector<std::vector<unsigned> > &covered_by,
    std::vector<unsigned> &chosen)
{
    unsigned left = covered_by.size();
    while (left > 0) {
        unsigned best_size = 0;
        unsigned best_coverage = 0;
        unsigned best_cid = UNASSIGNED;
        for (const unsigned &elem : superset) {
            if (num_covered[elem] > best_coverage
                || (num_covered[elem] == best_coverage
                    && m_hc->get_conjunction_size(elem) < best_size)) {
                best_size = m_hc->get_conjunction_size(elem);
                best_coverage = num_covered[elem];
                best_cid = elem;
            }
        }
        assert(best_cid == UNASSIGNED || best_coverage > 0);
        if (best_cid == UNASSIGNED) {
            break;
        }
        for (const unsigned &succ : covers[best_cid]) {
            if (chosen[succ] == UNASSIGNED) {
                left--;
                for (const unsigned &elem : covered_by[succ]) {
                    num_covered[elem]--;
                }
            }
            chosen[succ] = best_cid;
        }
        assert(num_covered[best_cid] == 0);
    }
    return left;
}

void HCNeighborsRefinement::compute_conflict(
    const std::vector<unsigned> &subgoal,
    std::vector<unsigned> &conflict,
    int bound)
{
    assert(conflict.empty() && m_satisfied_conjunctions.empty());
    // assert(m_hc->is_subgoal_reachable(subgoal));
    // assert(is_subgoal_covered_by_all_successors(subgoal));

    get_previously_statisfied_conjunctions(subgoal, m_satisfied_conjunctions);
    // m_hc->get_satisfied_conjunctions(subgoal, m_satisfied_conjunctions);
    for (const unsigned &cid : m_satisfied_conjunctions) {
        assert(cid < m_num_conjunctions_before_refinement);
        m_num_covered_by[cid] = 0;
        for (auto it = m_conjunction_to_successors[cid].lower_bound(bound);
                it != m_conjunction_to_successors[cid].end();
                it++) {
            m_num_covered_by[cid] += it->second.size();
        }
    }
#ifndef NDEBUG
    bool handled_all = !
#endif
    collect_greedy_minimal(m_satisfied_conjunctions,
                           m_num_covered_by,
                           m_conjunction_to_successors,
                           m_successor_to_conjunctions,
                           m_chosen,
                           bound);
    assert(handled_all);
    for (unsigned succ = 0; succ < m_num_successors; succ++) {
        if (m_chosen[succ] != UNASSIGNED) {
            unsigned cid = m_chosen[succ];
            assert(cid < m_num_conjunctions_before_refinement);
            set_utils::inplace_union(conflict,
                                     m_hc->get_conjunction(cid));
            for (auto it = m_conjunction_to_successors[cid].lower_bound(bound);
                    it != m_conjunction_to_successors[cid].end();
                    it++) {
                for (const auto& succ : it->second) {
                    m_chosen[succ] = UNASSIGNED;
                }
            }
        }
    }
    m_satisfied_conjunctions.clear();

    for (const unsigned &p : subgoal) {
        m_num_covered_by[p] = m_fact_to_negated_component[p].size();
    }
    if (collect_greedy_minimal(conflict,
                               m_num_covered_by,
                               m_fact_to_negated_component,
                               m_negated_component_to_facts,
                               m_chosen)) {
        collect_greedy_minimal(subgoal,
                               m_num_covered_by,
                               m_fact_to_negated_component,
                               m_negated_component_to_facts,
                               m_chosen);
    }
    for (unsigned state = 0; state < m_component_size; state++) {
        if (m_chosen[state] != UNASSIGNED) {
            unsigned p = m_chosen[state];
            assert(p < strips::num_facts());
            set_utils::insert(conflict, p);
            for (const unsigned &cov : m_fact_to_negated_component[p]) {
                m_chosen[cov] = UNASSIGNED;
            }
        }
    }

    assert(std::includes(subgoal.begin(), subgoal.end(),
                         conflict.begin(), conflict.end()));
#ifndef NDEBUG
    get_previously_statisfied_conjunctions(conflict, m_satisfied_conjunctions);
    std::vector<int> x(m_num_successors, 0);
    for (const auto& c : m_satisfied_conjunctions) {
        for (auto it = m_conjunction_to_successors[c].begin();
                it != m_conjunction_to_successors[c].end();
                it++) {
            for (const auto& succ : it->second) {
                x[succ] = std::max(x[succ], it->first);
            }
        }
    }
    for (unsigned i = 0; i < x.size(); i++) {
        assert(x[i] >= bound);
    }
    m_satisfied_conjunctions.clear();
#endif
    // assert(is_subgoal_covered_by_all_successors(conflict));
    // assert(!is_contained_in_component(conflict));
}

void HCNeighborsRefinement::print_statistics() const
{
    std::cout << "Total time spent on hC neighbors refinement: "
           << get_refinement_timer() << std::endl;
    std::cout << "Number of hC neighbors refinements: " << m_num_refinements << std::endl;
}

void HCNeighborsRefinement::add_options_to_parser(OptionParser &parser)
{
    HCHeuristicRefiner::add_options_to_parser(parser);
}

}
}

static std::shared_ptr<conflict_driven_learning::HeuristicRefiner>
_parse(options::OptionParser& parser)
{
    conflict_driven_learning::hc_heuristic::HCNeighborsRefinement::add_options_to_parser(parser);
    options::Options opts = parser.parse();
    if (!parser.dry_run()) {
        return std::make_shared<conflict_driven_learning::hc_heuristic::HCNeighborsRefinement>(opts);
    }
    return nullptr;
}

static PluginShared<conflict_driven_learning::HeuristicRefiner> _plugin("hcrn", _parse);
