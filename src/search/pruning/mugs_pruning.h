#ifndef MUGS_PRUNING_H
#define MUGS_PRUNING_H

#include <unordered_set>
#include "../pruning_method.h"
#include "../task_proxy.h"
#include "../heuristics/max_heuristic.h"

class GlobalState;

namespace mugs_pruning {
class MugsPruning : public PruningMethod {

int num_goal_facts;
uint hard_goals;
bool all_soft_goals;
std::unordered_set<uint> msgs;
std::vector<std::string> goal_fact_names;
Evaluator* max_heuristic;
int pruned_states = 0;

private:
    bool superset_contained(uint goal_subset, const std::unordered_set<uint> &set) const;
    bool is_superset(uint super, uint sub) const;
    bool insert_new_superset(uint goal_subset,  std::unordered_set<uint> &set) const;
    bool insert_new_subset(uint goal_subset,  std::unordered_set<uint> &set) const;
    std::unordered_set<uint> unsolvable_subgoals() const;
    std::unordered_set<uint> minimal_unsolvable_subgoals(std::unordered_set<uint> &ugs) const;
    void print_set(std::unordered_set<uint> s) const;
    void print_mugs() const;


public:
    explicit MugsPruning(const options::Options &opts);

    virtual void initialize(const std::shared_ptr<AbstractTask> &) override;
    virtual void prune_operators(const State &,std::vector<OperatorID> &) override {}
    virtual bool prune_state(const State &state) override;
    virtual void print_statistics() const override;

    
};
}

#endif