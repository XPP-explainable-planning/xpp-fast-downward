#include "state_minimization_nogoods.h"

#include "strips_compilation.h"
#include "../abstract_task.h"

#include <algorithm>
#include <iostream>
#include <cstdio>

namespace conflict_driven_learning
{
namespace hc_heuristic
{

bool StateMinimizationNoGoods::evaluate(
    const std::vector<unsigned> &conjunction_ids)
{
    return m_formula.contains_subset_of(conjunction_ids);
}

void StateMinimizationNoGoods::initialize()
{
    assert(m_full_goal.empty());
    const auto& goal = strips::get_task().get_goal();
    m_full_goal.insert(m_full_goal.end(),
                       goal.begin(),
                       goal.end());
}

void StateMinimizationNoGoods::refine(
    const PartialState &state)
{
    static std::vector<unsigned> goal_conjunctions;

    bool term = m_hc->set_early_termination(false);
    for (int var = m_task.get_num_variables() - 1; var >= 0; var--) {
        for (int val = 0; val < m_task.get_variable_domain_size(var); val++) {
            if (!state.is_defined(var) || val != state[var]) {
                m_new_facts.push_back(strips::get_fact_id(var, val));
            }
        }
        int res = m_hc->compute_heuristic_incremental(
                m_new_facts,
                m_reachable_conjunctions);
        if (res != HCHeuristic::DEAD_END) {
            m_hc->revert_incremental_computation(m_new_facts, m_reachable_conjunctions);
            m_clause.push_back(strips::get_fact_id(var, state[var]));
        }
        m_new_facts.clear();
        m_reachable_conjunctions.clear();
    }
    m_hc->set_early_termination(term);

    std::sort(m_clause.begin(), m_clause.end());
    m_formula.insert(m_clause);

    unsigned id = m_clauses.size();
    m_clauses.push_back(std::vector<unsigned>());
    m_clause.swap(m_clauses[id]);

    m_hc->get_satisfied_conjunctions(m_full_goal, goal_conjunctions);
    for (const auto& conj_id : goal_conjunctions) {
        if (!m_hc->get_conjunction_data(conj_id).achieved()) {
            m_conjs_to_clauses[conj_id].push_back(id);
        }
    }
    goal_conjunctions.clear();
}

void StateMinimizationNoGoods::synchronize_goal()
{
    static std::vector<bool> x;
    static std::vector<unsigned> goal_conjunctions;
    x.resize(m_clauses.size());
    std::fill(x.begin(), x.end(), false);

    m_formula.clear();
    m_hc->get_satisfied_conjunctions(strips::get_task().get_goal(),
                                     goal_conjunctions);
    for (const unsigned& conj_id : goal_conjunctions) {
        for (const auto& id : m_conjs_to_clauses[conj_id]) {
            if (!x[id]) {
                x[id] = true;
                m_formula.insert(m_clauses[id]);
            }
        }
    }
    goal_conjunctions.clear();
}

void StateMinimizationNoGoods::print_statistics() const
{
    printf("hC-nogood (state minimization) size: %zu\n", m_formula.size());
    std::cout << "hC-nogood (state minimization) evaluation time: "
           << get_evaluation_timer() << std::endl;
    std::cout << "hC-nogood (state minimization) refinement time: "
           << get_refinement_timer() << std::endl;
}

}
}