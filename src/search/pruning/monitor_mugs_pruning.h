#ifndef MONITOR_MUGS_PRUNING_H
#define MONITO_MUGS_PRUNING_H

#include <unordered_set>
#include "../pruning_method.h"
#include "../task_proxy.h"
#include "../heuristics/max_heuristic.h"
#include "../monitoring/monitor.h"

class GlobalState;

namespace monitor_mugs_pruning {
class MonitorMugsPruning : public PruningMethod {

bool all_soft_goals = false;
bool prune = true;
std::vector<std::string> goal_fact_names;
int pruned_states = 0;
int pruned_states_props = 0;
Evaluator* max_heuristic;
vector<Monitor*> monitors;

protected:
    bool msgs_changed = false;
    int num_goal_facts = 0;
    uint hard_goals = 0;
    std::unordered_set<uint> msgs;

    bool superset_contained(uint goal_subset, const std::unordered_set<uint> &set) const;
    bool is_superset(uint super, uint sub) const;
    bool insert_new_superset(uint goal_subset,  std::unordered_set<uint> &set, bool& inserted) const;
    bool insert_new_subset(uint goal_subset,  std::unordered_set<uint> &set) const;
    std::unordered_set<uint> unsolvable_subgoals(int number) const;
    std::unordered_set<uint> minimal_unsolvable_subgoals(std::unordered_set<uint> &ugs) const;
    virtual bool check_reachable(const State &state);
    void add_goal_to_msgs(const State &state);
    void print_set(std::unordered_set<uint> s) const;
    void print_mugs() const;


public:
    explicit MonitorMugsPruning(const options::Options &opts);

    virtual void initialize(const std::shared_ptr<AbstractTask> &) override;
    virtual void prune_operators(const State &state, std::vector<OperatorID> &ops) override;
    virtual bool prune_state(StateID parent_id, const GlobalState &state) override;
    virtual bool prune_init_state(const GlobalState &state) override;
    virtual bool prune_state(const State &state) override;
    virtual void print_statistics() const override;

    
};
}

#endif
