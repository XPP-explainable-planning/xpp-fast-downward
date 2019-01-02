#include "trap_unsat_heuristic.h"
#include "strips_compilation.h"
#include "partial_state_evaluator.h"
#include "set_utils.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../global_state.h"
#include "../utils/timer.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <cstdio>
#include <set>

namespace conflict_driven_learning {
namespace traps {

TrapUnsatHeuristic::TrapUnsatHeuristic(const options::Options& opts)
    : Heuristic(opts)
{
    strips::initialize(*task);
    m_task = &strips::get_task();
    m_formula.set_num_keys(strips::num_facts());
    m_formula_all.set_num_keys(strips::num_facts());
    std::vector<Evaluator*> evals = opts.get_list<Evaluator*>("evaluators");
    for (unsigned i = 0; i < evals.size(); i++) {
        m_evaluators.push_back(dynamic_cast<PartialStateEvaluator*>(evals[i]));
        assert(m_evaluators.back() != nullptr);
    }
    initialize(opts.get<int>("k"));
}

void
TrapUnsatHeuristic::add_options_to_parser(options::OptionParser& parser)
{
    parser.add_option<int>("k", "", "0");
    parser.add_list_option<Evaluator *>("evaluators", "", "[]");
    Heuristic::add_options_to_parser(parser);
}

void
TrapUnsatHeuristic::initialize(unsigned k)
{
    std::cout << "Initializating traps heuristic ..." << std::endl;
    utils::Timer initialiation_t;

    m_action_post.resize(m_task->num_actions());
    for (unsigned op = 0; op < m_action_post.size(); op++) {
        const strips::Action &action = m_task->get_action(op);
        std::set_union(action.pre.begin(), action.pre.end(),
                       action.add.begin(), action.add.end(),
                       std::back_inserter(m_action_post[op]));
        set_utils::inplace_difference(m_action_post[op], action.del);
        assert(std::is_sorted(m_action_post[op].begin(), m_action_post[op].end()));
        assert(std::unique(m_action_post[op].begin(),
                           m_action_post[op].end()) == m_action_post[op].end());
        assert(!m_task->contains_mutex(m_action_post[op]));
    }

    m_is_mutex.resize(m_task->num_actions());
    for (unsigned i = 0; i < m_is_mutex.size(); i++) {
        m_is_mutex[i].resize(strips::num_facts(), false);
        const strips::Action &action = m_task->get_action(i);
        for (unsigned q = 0; q < m_is_mutex[i].size(); q++) {
            m_is_mutex[i][q] = m_task->are_mutex(q, action.pre) ||
                               (!set_utils::contains(action.del, q) && m_task->are_mutex(q, action.add));
        }
    }

    if (k > 0) {
        std::cout << "Generating k<=" << k << " conjunctions (t=" << initialiation_t << ") ..." << std::endl;
        std::vector<bool> fact_mutex_with_goal(strips::num_facts(), false);
        for (unsigned p = 0; p < fact_mutex_with_goal.size(); p++) {
            fact_mutex_with_goal[p] = m_task->are_mutex(p, m_task->get_goal());
        }
        std::vector<bool> in_goal(task->get_num_variables(), false);
        for (int i = 0; i < task->get_num_goals(); i++) {
            in_goal[task->get_goal_fact(i).var] = true;
        }

        unsigned num_mutex = 0;
        struct EnumData {
            int var;
            int succ_var;
            int val;
            bool goalvar;
            bool is_goal_mutex;
            EnumData(int var, int domain, bool goalvar, bool mut)
                : var(var), succ_var(var), val(domain), goalvar(goalvar), is_goal_mutex(mut)
            {}
        };
        std::vector<EnumData> vars;
        std::vector<unsigned> fact_ids;
        for (int var = 0; var < task->get_num_variables(); var++) {
            vars.emplace_back(var, task->get_variable_domain_size(var), in_goal[var], false);
            fact_ids.push_back(-1);
            while (!vars.empty()) {
                EnumData& e = vars.back();
                fact_ids.pop_back();
                if (--e.val < 0) {
                    vars.pop_back();
                    continue;
                }
                unsigned fact_id = strips::get_fact_id(e.var, e.val);
                bool goal_mutex = fact_mutex_with_goal[fact_id];
                fact_ids.push_back(fact_id);
                if (e.goalvar) {
                    m_conjunctions.push_back(fact_ids);
                    m_formula_all.insert(fact_ids);
                    m_mutex_with_goal.push_back(e.is_goal_mutex || goal_mutex);
                    if (m_mutex_with_goal.back()) { num_mutex++; }
                }
                if (fact_ids.size() < k
                        && ++e.succ_var < task->get_num_variables()) {
                    vars.emplace_back(
                            e.succ_var,
                            task->get_variable_domain_size(e.succ_var),
                            e.goalvar || in_goal[e.succ_var],
                            e.is_goal_mutex || goal_mutex);
                    fact_ids.push_back(-1);
                }
            }
            assert(fact_ids.empty());
        }
        std::cout << "Generated " << m_conjunctions.size() << " conjunctions, "
            << num_mutex << " are mutex with the goal" << std::endl;

        std::cout << "Computing transition relation (t=" << initialiation_t
            << ") ..." << std::endl;
        m_dest_to_transition_ids.resize(m_conjunctions.size());
        m_src_to_transition_ids.resize(m_conjunctions.size());
        std::vector<unsigned> progress;
        unsigned num_transitions = 0;
        for (unsigned i = 0; i < m_conjunctions.size(); i++) {
            const std::vector<unsigned>& conj = m_conjunctions[i];
            for_every_progression_action(conj, [&](unsigned op) {
                progression(conj, op, progress);
                m_hyperarcs.push_back(ForwardHyperTransition(op, i));
                ForwardHyperTransition& arc = m_hyperarcs[num_transitions];
                assert(!m_task->contains_mutex(progress));
                unsigned num = 0;
                m_formula_all.forall_subsets(progress, [&](unsigned id) {
                    m_dest_to_transition_ids[id].push_back(num_transitions);
                    arc.destinations.push_back(id);
                    num++;
                });
                std::sort(arc.destinations.begin(), arc.destinations.end());
                m_src_to_transition_ids[i].push_back(num_transitions);
                progress.clear();
                m_num_post_conjunctions.push_back(num);
                m_transition_source.push_back(i);
                if (num == 0) {
                    m_no_post_transitions.push_back(num_transitions);
                }
                num_transitions++;
                return false;
            });
            std::sort(m_src_to_transition_ids[i].begin(), m_src_to_transition_ids[i].end(), [&](const unsigned& i, const unsigned& j) {
                return m_hyperarcs[i].label < m_hyperarcs[j].label;
            });
        }
        std::cout << "Found " << num_transitions << " transitions, of which "
            << m_no_post_transitions.size() << " have empty post set" << std::endl;

        std::cout << "Performing reachability analysis (t=" << initialiation_t << ")" << std::endl;
        propagate_reachability_setup_formula();
    }

    std::cout << "Initialized trap with " << m_formula.size()
              << " conjunctions after " << initialiation_t
              << std::endl;
}

void
TrapUnsatHeuristic::set_abstract_task(std::shared_ptr<AbstractTask> task)
{
    // assumes that abstract task is updated for all evaluators beforehand
    strips::update_goal_set(*task);
    std::fill(m_mutex_with_goal.begin(), m_mutex_with_goal.end(), false);
    std::vector<bool> fact_mutex_with_goal(strips::num_facts(), false);
    for (unsigned p = 0; p < fact_mutex_with_goal.size(); p++) {
        fact_mutex_with_goal[p] = m_task->are_mutex(p, m_task->get_goal());
    }
    for (int i = m_conjunctions.size() - 1; i >= 0; i--) {
        const std::vector<unsigned>& conj = m_conjunctions[i];
        for (int j = conj.size() - 1; j >= 0; j--) {
            if (fact_mutex_with_goal[conj[j]]) {
                m_mutex_with_goal[i] = true;
                break;
            }
        }
    }
    m_formula.clear();
    propagate_reachability_setup_formula();
}

int
TrapUnsatHeuristic::compute_heuristic(const std::vector<unsigned> &state)
{
    if (m_formula.contains_subset_of(state)) {
        return DEAD_END;
    }
    return 0;
}

int
TrapUnsatHeuristic::compute_heuristic(const GlobalState &state)
{
    static std::vector<unsigned> state_fact_ids;
    state_fact_ids.clear();
    for (int var = 0; var < task->get_num_variables(); var++) {
        state_fact_ids.push_back(strips::get_fact_id(var, state[var]));
    }
    return compute_heuristic(state_fact_ids);
}

bool
TrapUnsatHeuristic::are_dead_ends(const std::vector<unsigned> &phi)
{
    if (m_evaluators.empty()) {
        return false;
    }
    std::vector<std::pair<int, int> > partial_state;
    partial_state.reserve(phi.size());
    for (const unsigned &p : phi) {
        partial_state.push_back(strips::get_variable_assignment(p));
    }
    bool res = false;
    for (unsigned i = 0; i < m_evaluators.size(); i++) {
        if (m_evaluators[i]->evaluate_partial_state(partial_state) == DEAD_END) {
            res = true;
            break;
        }
    }
    return res;
}

void
TrapUnsatHeuristic::progression(const std::vector<unsigned> &phi,
                                     unsigned op,
                                     std::vector<unsigned> &post)
{
    assert(op < m_task->num_actions());
    assert(op < m_action_post.size());
    const strips::Action &action = m_task->get_action(op);
    std::set_difference(phi.begin(), phi.end(),
                        action.del.begin(), action.del.end(),
                        std::back_inserter(post));
    set_utils::inplace_union(post, m_action_post[op]);
    assert(!m_task->contains_mutex(post));
    assert(phi != post);
}

bool
TrapUnsatHeuristic::are_mutex(const std::vector<unsigned> &phi,
                                   unsigned op) const
{
    assert(op < m_is_mutex.size());
    const std::vector<bool> &mutex = m_is_mutex[op];
    for (const unsigned &p : phi) {
        assert(p < mutex.size());
        if (mutex[p]) {
            return true;
        }
    }
    return false;
}

bool
TrapUnsatHeuristic::for_every_progression_action(
        const std::vector<unsigned>& phi,
        std::function<bool(unsigned)> callback)
{
    static std::vector<bool> closed;
    closed.resize(m_task->num_actions());
    std::fill(closed.begin(), closed.end(), false);
    bool skip = false;
    for (const unsigned& p : phi) {
        const std::vector<unsigned>& ops = m_task->get_actions_with_del(p);
        for (const unsigned& op : ops) {
            if (!closed[op]) {
                closed[op] = true;
                if (!are_mutex(phi, op)) {
                    if (callback(op)) {
                        skip = true;
                        break;
                    }
                }
            }
        }
        if (skip) {
            break;
        }
    }
    return skip;
}

bool
TrapUnsatHeuristic::for_every_regression_action(
        const std::vector<unsigned>& conj,
        std::function<bool(unsigned)> callback)
{
    static std::vector<bool> closed;
    closed.resize(m_task->num_actions());
    std::fill(closed.begin(), closed.end(), false);
    for (const unsigned& p : conj) {
        const std::vector<unsigned>& ops = m_task->get_actions_with_del(p);
        for (const unsigned& op : ops) {
            closed[op] = true;
        }
    }
    for (const unsigned& p : conj) {
        const std::vector<unsigned>& ops = m_task->get_actions_with_add(p);
        bool done = false;
        for (const unsigned& op : ops) {
            if (!closed[op]) {
                closed[op] = true;
                if (callback(op)) {
                    done = true;
                    break;
                }
            }
        }
        if (done) {
            return true;
        }
    }
    return false;
}

void
TrapUnsatHeuristic::propagate_reachability_setup_formula()
{
    m_goal_reachable.resize(m_conjunctions.size());
    std::fill(m_goal_reachable.begin(), m_goal_reachable.end(), 1);

    if (m_evaluators.size() > 0) {
        for (unsigned i = 0; i < m_conjunctions.size(); i++) {
            if (m_goal_reachable[i] == 0 && are_dead_ends(m_conjunctions[i])) {
                m_goal_reachable[i] = -1;
                m_formula.insert(m_conjunctions[i]);
            }
        }
    }

    update_reachability_insert_conjunctions();
}


void
TrapUnsatHeuristic::update_reachability_insert_conjunctions()
{
    std::vector<bool> was_unreachable(m_conjunctions.size(), false);

    std::vector<unsigned> exploration_queue;
    std::vector<unsigned> open(m_num_post_conjunctions);
    for (unsigned i = 0; i < m_mutex_with_goal.size(); i++) {
        was_unreachable[i] = m_goal_reachable[i] <= 0;
        if (m_goal_reachable[i] == -1) {
            continue;
        }
        m_goal_reachable[i] = 0;
        if (!m_mutex_with_goal[i]) {
            exploration_queue.push_back(i);
            m_goal_reachable[i] = 1;
        }
    }
    for (unsigned t : m_no_post_transitions) {
        int src = m_transition_source[t];
        if (m_goal_reachable[src] == 0) {
            m_goal_reachable[src] = 1;
            exploration_queue.push_back(src);
        }
    }
    while (!exploration_queue.empty()) {
        unsigned dest = exploration_queue.back();
        exploration_queue.pop_back();
        const std::vector<unsigned>& ts = m_dest_to_transition_ids[dest];
        for (unsigned t : ts) {
            if (--open[t] == 0) {
                unsigned src = m_transition_source[t];
                if (m_goal_reachable[src] == 0) {
                    m_goal_reachable[src] = 1;
                    exploration_queue.push_back(src);
                }
            }
        }
    }

    for (unsigned i = 0; i < m_conjunctions.size(); i++) {
        if (!was_unreachable[i] && m_goal_reachable[i] <= 0) {
            m_formula.insert(m_conjunctions[i]);
        }
    }
}

TrapUnsatHeuristic::Formula&
TrapUnsatHeuristic::get_all_conjunctions_formula()
{
    return m_formula_all;
}

const TrapUnsatHeuristic::Formula&
TrapUnsatHeuristic::get_all_conjunctions_formula() const
{
    return m_formula_all;
}

bool
TrapUnsatHeuristic::can_reach_goal(unsigned conjid) const
{
    return m_goal_reachable[conjid] >= 1;
}

std::pair<unsigned, bool>
TrapUnsatHeuristic::insert_conjunction(const std::vector<unsigned>& conj)
{
#ifndef NDEBUG
    bool mutex = false;
    for (unsigned i = 0; i < conj.size(); i++) {
        mutex = mutex || m_task->are_mutex(conj[i], m_task->get_goal());
    }
    assert(mutex);
#endif
    unsigned conj_id = -1;
    m_formula_all.forall_subsets(conj, [&](unsigned id) {
        if (m_conjunctions[id].size() == conj.size()) {
            conj_id = id;
        }
    });
    if (conj_id != (unsigned) -1) {
        assert(m_mutex_with_goal[conj_id]);
        if (m_goal_reachable[conj_id] == 1) {
            m_goal_reachable[conj_id] = 0;
            m_formula.insert(conj);
        }
        return std::pair<unsigned, bool>(conj_id, false);
    } else {
        m_conjunctions.push_back(conj);
        m_formula.insert(conj);
        m_formula_all.insert(conj);
        m_mutex_with_goal.push_back(true);
        m_goal_reachable.push_back(0);
        m_src_to_transition_ids.push_back(std::vector<unsigned>{});
        m_dest_to_transition_ids.push_back(std::vector<unsigned>{});
        return std::pair<unsigned, bool>(m_conjunctions.size() - 1, true);
    }
}

void
TrapUnsatHeuristic::set_transitions(
        unsigned conj_id,
        std::vector<ForwardHyperTransition>&& transitions)
{
    if (transitions.empty()) {
        return;
    }
    std::sort(transitions.begin(), transitions.end(), [](const ForwardHyperTransition& t1, const ForwardHyperTransition& t2) {
        return t1.label < t2.label;
    });
    std::vector<unsigned>& ts = m_src_to_transition_ids[conj_id];
    if (ts.empty()) {
        int i = m_hyperarcs.size();
        unsigned num_transitions = m_hyperarcs.size() + transitions.size();
        m_hyperarcs.resize(num_transitions, ForwardHyperTransition(-1, -1));
        m_transition_source.resize(num_transitions, conj_id);
        m_num_post_conjunctions.resize(num_transitions, 0);
        for (unsigned j = 0; j < transitions.size(); j++) {
            m_hyperarcs[i] = std::move(transitions[j]);
            ForwardHyperTransition& t = m_hyperarcs[i];
            std::sort(t.destinations.begin(), t.destinations.end());
            m_num_post_conjunctions[i] = t.destinations.size();
            for (const unsigned& dest : t.destinations) {
                m_dest_to_transition_ids[dest].push_back(i);
            }
            if (t.destinations.empty()) {
                m_no_post_transitions.push_back(i);
            }
            ts.push_back(i);
            i++;
        }
    } else {
        assert(transitions.size() == ts.size());
        for (unsigned i = 0; i < ts.size(); i++) {
            unsigned idx = ts[i];
            assert(m_hyperarcs[idx].label == transitions[i].label);
            assert(transitions[i].destinations.size() >= m_num_post_conjunctions[idx]);
            bool was_empty = m_num_post_conjunctions[i] == 0;
            if (m_num_post_conjunctions[i] == transitions[i].destinations.size()) {
                assert(m_hyperarcs[idx] == transitions[i]);
                continue;
            }
            m_hyperarcs[idx] = std::move(transitions[i]);
            m_num_post_conjunctions[idx] = m_hyperarcs[idx].destinations.size();
            for (const unsigned& dest : m_hyperarcs[idx].destinations) {
                set_utils::insert(m_dest_to_transition_ids[dest], idx);
            }
            if (was_empty) {
                assert(m_num_post_conjunctions[idx] > 0);
                set_utils::remove(m_no_post_transitions, idx);
            }
        }
    }
}

bool
TrapUnsatHeuristic::evaluate_check_dead_end(const GlobalState& state)
{
    return compute_heuristic(state) == DEAD_END;
}

bool
TrapUnsatHeuristic::add_to_transition_post(
        const unsigned& source,
        const unsigned& op,
        const unsigned& dest)
{
    const std::vector<unsigned>& ts = m_src_to_transition_ids[source];
    auto it = std::lower_bound(ts.begin(), ts.end(), op, [&](const unsigned& x, const unsigned& y) {
        assert(y == op);
        return m_hyperarcs[x].label < y;
    });
    if (it == ts.end() || m_hyperarcs[*it].label != op) {
        return false;
    }
    unsigned idx = *it;
    m_hyperarcs[idx].destinations.push_back(dest);
    assert(set_utils::is_set(m_hyperarcs[idx].destinations));
    m_num_post_conjunctions[idx]++;
    m_dest_to_transition_ids[dest].push_back(idx);
    return true;
}

bool
ForwardHyperTransition::operator==(const ForwardHyperTransition& other) const
{
    if (label != other.label || source != other.source) {
        return false;
    }
    std::set<unsigned> d1(destinations.begin(), destinations.end());
    std::set<unsigned> d2(other.destinations.begin(), other.destinations.end());
    return d1 == d2;
}

}
}

static Heuristic*
_parse(options::OptionParser& parser)
{
    conflict_driven_learning::traps::TrapUnsatHeuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (!parser.dry_run()) {
        opts.set<bool>("cache_estimates", false);
        return new conflict_driven_learning::traps::TrapUnsatHeuristic(opts);
    }
    return nullptr;
}

static Plugin<Evaluator> _plugin("traps", _parse);
