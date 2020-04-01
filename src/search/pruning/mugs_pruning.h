#ifndef MUGS_PRUNING_H
#define MUGS_PRUNING_H

#include <unordered_set>
#include "../pruning_method.h"
#include "../task_proxy.h"
#include "../heuristics/max_heuristic.h"

class GlobalState;

namespace mugs_pruning {
class MugsPruning : public PruningMethod {

// flag which indicates if all goal facts should be treated as soft goals
bool all_soft_goals = false;
bool prune = true;
bool use_cost_bound_reachable = false;
// current cost bound
int cost_bound;
std::vector<std::string> goal_fact_names;
Evaluator* max_heuristic;
int pruned_states = 0;

protected:
    // TODO
    bool msgs_changed = false;
    // total number of goal facts
    int num_goal_facts = 0;
    // number of hard goals
    uint hard_goals = 0;
    // set of maximal solvable goal subsets
    std::unordered_set<uint> msgs;

    /**
     * Checks if any of the sets in @set in a superset of @goal_set
     * @param goal_subset
     * @param set
     * @return
     */
    bool superset_contained(uint goal_subset, const std::unordered_set<uint> &set) const;

    /**
     * Checks if @sub is a subset of @super
     * @param super
     * @param sub
     * @return
     */
    bool is_superset(uint super, uint sub) const;

    /**
     * Inserts @goal_subset into @set (if no superset is already contained)
     * and removes all sets from @set which are a subset of @goal_subset
     * @param goal_subset
     * @param set
     * @param inserted true if @goal_subset was inserted
     * @return true if a subset of @goal_subset was already contained in @set
     */
    bool insert_new_superset(uint goal_subset,  std::unordered_set<uint> &set, bool& inserted) const;

    /**
     * Inserts @goal_subset into @set (if no subset is already contained)
     * and removes all sets from @set which are a superset of @goal_subset
     * @param goal_subset
     * @param set
     * @return
     */
    bool insert_new_subset(uint goal_subset,  std::unordered_set<uint> &set) const;

    /**
     * Compute all unsolvable goal subsets based on the maximal solvable sub goals.
     * The unsolvable goal subsets are not minimal.
     * @return the computed unsolvable goal subsets
     */
    std::unordered_set<uint> unsolvable_subgoals() const;

    /**
     * compute the MUGS based on the given set @ugs of unsolvable goal subsets
     * @param ugs
     * @return MUGS
     */
    std::unordered_set<uint> minimal_unsolvable_subgoals(std::unordered_set<uint> &ugs) const;

    /**
     * Check if a superset of the already reached msgs is reachable from @state.
     * Returns false if not all hard goals are reachable.
     * @param state
     * @return false if either not all hard goals or no larger msgs is reachable
     */
    virtual bool check_reachable(const State &state);

    /**
     * Adds the goal facts currently satisfied in @state (cg) to the msgs
     * if no super set of cg in contained in msgs
     * @param state
     */
    void add_goal_to_msgs(const State &state);


    void print_set(std::unordered_set<uint> s) const;
    void print_mugs() const;


public:
    explicit MugsPruning(const options::Options &opts);

    void initialize(const std::shared_ptr<AbstractTask> &) override;
    void prune_operators(const State &state, std::vector<OperatorID> &ops) override;
    bool prune_state(const State &state) override;
    void print_statistics() const override;

    
};
}

#endif
