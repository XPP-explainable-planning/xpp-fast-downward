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

void StateMinimizationNoGoods::refine(
    const PartialState &state)
{
    bool term = m_hc->set_early_termination(false);
    int res;
    //for (int var = g_variable_domain.size() - 1; var >= 0; var--) {
    for (int var = m_task.get_num_variables() - 1; var >= 0; var--) {
        if (!state.is_defined(var)) {
            continue;
        }
        for (int val = 0; val < m_task.get_variable_domain_size(var); val++) {
            if (val != state[var]) {
                m_new_facts.push_back(strips::get_fact_id(var, val));
            }
        }
        res = m_hc->compute_heuristic_incremental(m_new_facts,
                m_reachable_conjunctions);
        if (res != HCHeuristic::DEAD_END) {
            m_hc->revert_incremental_computation(m_new_facts, m_reachable_conjunctions);
            m_clause.push_back(strips::get_fact_id(var, state[var]));
        }
        m_new_facts.clear();
        m_reachable_conjunctions.clear();
    }
    std::sort(m_clause.begin(), m_clause.end());
    m_formula.insert(m_clause);
    m_clause.clear();
    m_hc->set_early_termination(term);
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
