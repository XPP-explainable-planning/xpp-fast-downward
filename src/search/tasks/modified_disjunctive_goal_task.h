#ifndef TASKS_MODIFIED_DISJUNCTIVE_GOALS_TASK_H
#define TASKS_MODIFIED_DISJUNCTIVE_GOALS_TASK_H

#include "delegating_task.h"

#include <vector>

namespace extra_tasks {
class ModifiedDisjunctiveGoalTask : public tasks::DelegatingTask {
    const std::vector<FactPair> and_goals;
    const std::vector<FactPair> or_goals;

    const FactPair new_goal;


public:
    ModifiedDisjunctiveGoalTask(
        const std::shared_ptr<AbstractTask> &parent,
        const std::vector<FactPair> and_goals, const std::vector<FactPair> or_goals);
    ~ModifiedDisjunctiveGoalTask() = default;

    virtual int get_num_variables() const override;
    virtual std::string get_variable_name(int var) const override;
    virtual int get_variable_domain_size(int var) const override;
    virtual int get_variable_axiom_layer(int var) const override;
    virtual int get_variable_default_axiom_value(int var) const override;

    virtual int get_operator_cost(int index, bool is_axiom) const override;
    virtual std::string get_operator_name(int index, bool is_axiom) const override;
    virtual int get_num_operators() const override;
    virtual int get_num_operator_preconditions(int index, bool is_axiom) const override;
    virtual FactPair get_operator_precondition(
        int op_index, int fact_index, bool is_axiom) const override;
    virtual int get_num_operator_effects(int op_index, bool is_axiom) const override;
    virtual int get_num_operator_effect_conditions(
        int op_index, int eff_index, bool is_axiom) const override;
    virtual FactPair get_operator_effect_condition(
        int op_index, int eff_index, int cond_index, bool is_axiom) const override;
    virtual FactPair get_operator_effect(
        int op_index, int eff_index, bool is_axiom) const override;

    virtual int get_num_goals() const override;
    virtual FactPair get_goal_fact(int index) const override;

    virtual std::vector<int> get_initial_state_values() const override;
};
}

#endif
